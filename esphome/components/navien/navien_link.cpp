#include <cmath>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"


namespace esphome {
namespace navien {

static const char *TAG = "navien.link";

bool NavienCmd::change_observed(const RECV_BUFFER& recv_buffer) const{
  if (this->idx >= HDR_SIZE + recv_buffer.hdr.len)
    return false; // out of bounds

  ESP_LOGV(TAG, "met_by: dst %d<=>%d, value %d<=>%d",
      this->dst, recv_buffer.hdr.dst,
      this->expected, recv_buffer.raw_data[this->idx] & this->mask);

  if (recv_buffer.hdr.direction != PACKET_DIR_STATUS || recv_buffer.hdr.dst != this->dst)
    return false;

  uint8_t value = recv_buffer.raw_data[this->idx];
  if ((value & this->mask) != this->expected)
    return false;

  return true;
}

uint8_t NavienCmd::send(NavienUartI* uart) {
  uart->write_array(buffer, len);
  NavienLink::print_buffer(buffer, len);
  return --tries;
}

NavienLink *NavienLink::get_instance(NavienUartI *uart) {
  static NavienLink *instance = nullptr;
  if (instance == nullptr) {
    if (uart == nullptr) {
      ESP_LOGE(TAG, "NavienLink UART adapter required on first get_instance call");
      return nullptr;
    }
    instance = new NavienLink(uart);
  } else if (uart != nullptr && instance->uart != uart) {
    ESP_LOGW(TAG, "Ignoring request to replace Navien UART adapter after initialization");
  }
  return instance;
}


bool NavienLink::seek_to_marker(){
  if (uart == nullptr) {
    return false;
  }
  uint8_t byte;

  int available = uart->available();
  for (int i = 0; i < available; i++){
    uart->peek_byte(&byte);
    if (byte == PACKET_MARKER)
      return true;
    
    uart->read_byte(&byte);
  }
  return false;
}


void NavienLink::parse_control_packet(){
  ESP_LOGV(TAG, "Got Control Packet => %d bytes", this->recv_buffer.hdr.len + HDR_SIZE);
  //  Navien::print_buffer(this->recv_buffer.raw_data, this->recv_buffer.hdr.len + HDR_SIZE);

  if (std::memcmp(this->recv_buffer.raw_data, NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT)) == 0) {
    /* This is a NAVILINK_PRESENT_PKT that wasn't sent by us, so another NaviLink is also hooked up */
    if (!this->other_navilink_installed) {
      ESP_LOGW(TAG, "Detected NAVILINK_PRESENT packet from another NaviLink device, will stop sending NAVILINK_PRESENT packets until rebooted");
      this->other_navilink_installed = true;
    }

    /* There's another NaviLink present, and it just sent its presence packet, which means the bus is now clear and 
       we can safely send our commands without it stomping on us. When there's not another device we'll send the 
       command from parse_status_packet() instead.
       Note that we only send our commands when we see presence packets and not other control packets from the other 
       NaviLink so that there aren't two things telling the Navien unit to do two (possibly conflicting) things at the
       same time. */
    send_queued_cmd();
  }
}
  
void NavienLink::parse_status_packet(){
  check_cmd_complete();

  switch(this->recv_buffer.hdr.dst){
  case PACKET_DST_WATER:
    ESP_LOGD(TAG, "SRC:0x%02X B8: 0x%02X, B32: 0x%02X, r_enabled: 0x%02X",
             this->recv_buffer.hdr.src,
             this->recv_buffer.water.unknown_06,
             this->recv_buffer.water.unknown_32,
             this->recv_buffer.water.recirculation_enabled);
    for (uint8_t i = 0; i < NAVIEN_CASCADE_MAX; ++i)
      if (visitors_[i]) visitors_[i]->on_water(recv_buffer.water, recv_buffer.hdr.src);
    break;
  case PACKET_DST_GAS:
    ESP_LOGD(TAG, "SRC:0x%02X => Gas", this->recv_buffer.hdr.src);
    for (uint8_t i = 0; i < NAVIEN_CASCADE_MAX; ++i)
      if (visitors_[i]) visitors_[i]->on_gas(recv_buffer.gas, recv_buffer.hdr.src);
    break;
  }

  if (!this->other_navilink_installed) {
    /* There's no other NaviLink on the bus, send a command right after we see a status packet. If there is a NaviLink
       on the bus, we'll send it in parse_control_packet() after we see the presence packet from the other device. */
    send_queued_cmd();
  }
}
  
void NavienLink::check_cmd_complete() {
  if (!this->cmd_buffer.empty()){
    if (cmd_buffer.back().change_observed(this->recv_buffer)){
      /* The queued command has been processed. Replace the head command in the buffer with an acknowledgement, so
         that the acknowledgement will be sent next, before any other pending commands. */
      ESP_LOGV(TAG, "Command response received, sending acknowledgement");
      cmd_buffer.back() = NavienCmd(
        ACKNOWLEDGEMENT, sizeof(ACKNOWLEDGEMENT),
        0, 0, 0, 0,
        1 // acknowledgements don't get retried
      );
    }
  }
}

void NavienLink::send_queued_cmd() {
  if (!this->cmd_buffer.empty()){
    // Send the command
    NavienCmd &cmd = cmd_buffer.back();
    uint8_t tries_left = cmd.send(this->uart);

    if (tries_left == 0){
      // No more tries left, dequeue the command
      if (!cmd.is_ack()) { // don't log for acknowledgements
        ESP_LOGW(TAG, "Command change not seen after maximum retries");
      }
      cmd_buffer.pop_back();
    }
    // NOTE: Don't try to access cmd from here on, the above pop_back() makes it invalid!
  }
  else if (!this->other_navilink_installed){
    // If there's no pending command, send a NAVILINK_PRESENT packet so the unit knows we're here.
    // When the unit is in an automatic recirculation mode, this tell is that we're controlling 
    // when it does and does not recirculate (and it triggers the "Recirculation settings must be 
    // configured through the NaviLink app" message on the unit's front panel when you try to
    // change the recirculation setting).
    uart->write_array(NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT));
    NavienLink::print_buffer(NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT));
  }
}

void NavienLink::parse_packet(){
  uint8_t crc_c = 0x00;
  uint8_t crc_r = 0x00;

  //NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len + 1);
  crc_r = this->recv_buffer.raw_data[HDR_SIZE + this->recv_buffer.hdr.len];
  
  switch(this->recv_buffer.hdr.direction){
  case PACKET_DIR_STATUS: {
    uint16_t seed;
    if (this->recv_buffer.hdr.src == PACKET_SRC_STATUS) {
      seed = CHECKSUM_SEED_4B;
    } else {
      seed = CHECKSUM_SEED_62;
    }
    crc_c = NavienLink::checksum(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len, seed);
    if (crc_c != crc_r){
      ESP_LOGE(TAG, "SRC:0x%02X Status Packet checksum error: 0x%02X (calc) != 0x%02X (recv), seed=0x%02X", this->recv_buffer.hdr.src, crc_c, crc_r, seed);
      NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len + 1);
      break;
    }
    parse_status_packet();
    break;
  }
  case PACKET_DIR_CONTROL:
  /**
   * The condition below was obsrved in cascade setup where there are lots of 
   * messages, apparently between Navien units and that have some other checksum algorithm that 
   * we're yet to discover. For now we simply ignore those packets. 
   */
    if (this->recv_buffer.hdr.src != PACKET_SRC_CONTROL) {
      ESP_LOGD(TAG, "Control packet from SRC:0x%02X - we don't know how to handle it yet", this->recv_buffer.hdr.src);
      return;
    }
    crc_c = NavienLink::checksum(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len, CHECKSUM_SEED_62);
    if (crc_c != crc_r){
      ESP_LOGE(TAG, "SRC:0x%02X Control Packet checksum error: 0x%02X (calc) != 0x%02X (recv), seed=0x%02X", this->recv_buffer.hdr.src, crc_c, crc_r, CHECKSUM_SEED_62);
      this->on_error();
      NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len + 1);
      break;
    }
    parse_control_packet();
    break;
  }

  //  ESP_LOGV(TAG, "Calculated checksum over %d bytes => 0x%02X", HDR_SIZE + this->recv_buffer.hdr.len, crc);

}


void NavienLink::on_error() {
  ESP_LOGD(TAG, "Notifying visitors of communication error");
  for (uint8_t i = 0; i < NAVIEN_CASCADE_MAX; ++i) {
    if (visitors_[i]) {
      visitors_[i]->on_error();
    }
  }
}

  
void NavienLink::receive() {
  if (uart == nullptr) {
    ESP_LOGE(TAG, "UART pointer is null; skipping receive");
    return;
  }

  int available = uart->available();
  if (!available) {
    return;
  }

  ESP_LOGV(TAG, "%d bytes available", available);
  while (available) {
    switch (this->recv_state) {
      case INITIAL:
        if (this->seek_to_marker()) {
          this->recv_state = MARKER_FOUND;
          ESP_LOGV(TAG, "Marker Found");
          break;
        }
        // No marker found and no data left to read. Exit and wait for more bytes to come.
        return;
      case MARKER_FOUND:
        available = uart->available();
        if (available < HDR_SIZE) {
          ESP_LOGV(TAG, "Only %d bytes available - less than header size", available);
          return;
        }
        if (!uart->read_array(this->recv_buffer.raw_data, HDR_SIZE)) {
          ESP_LOGW(TAG, "Failed to read header");
          this->on_error();
          break;
        }
        this->recv_state = HEADER_PARSED;
        ESP_LOGV(TAG, "Parsed header. %d bytes of body left", this->recv_buffer.hdr.len);
        // fall through
      case HEADER_PARSED: {
        available = uart->available();

        // +1 here is for the checksum - it is in the last byte
        uint8_t len = this->recv_buffer.hdr.len + 1;
        if (available < len) {
          ESP_LOGV(TAG, "Only %d data bytes available - less than len", available);
          return;
        }
        if (!uart->read_array(this->recv_buffer.raw_data + HDR_SIZE, len)) {
          ESP_LOGW(TAG, "Failed to read %d bytes", len);
          this->on_error();
          break;
        }
        ESP_LOGV(TAG, "Got Packet => %d bytes", len + HDR_SIZE);

        // Navien::print_buffer(this->recv_buffer.raw_data, len + HDR_SIZE);
        this->parse_packet();
        available = uart->available();
        this->recv_state = INITIAL;
        break;
      }
    }
  }
}

void NavienLink::send_turn_on_cmd(){
  send_cmd(NavienCmd(
    TURN_ON_CMD, sizeof(TURN_ON_CMD),
    PACKET_DST_WATER, offsetof(WATER_DATA, system_power),
    POWER_STATUS_ON_OFF_MASK, POWER_STATUS_ON_OFF_MASK,
    20 // retry longer because sometimes the unit takes longer to respond to this command
  ));
}

void NavienLink::send_turn_off_cmd(){
  send_cmd(NavienCmd(
    TURN_OFF_CMD, sizeof(TURN_OFF_CMD),
    PACKET_DST_WATER, offsetof(WATER_DATA, system_power),
    POWER_STATUS_ON_OFF_MASK, 0x00
  ));
}

void NavienLink::send_hot_button_cmd(){
  send_cmd(NavienCmd(
    HOT_BUTTON_PRESS_CMD, sizeof(HOT_BUTTON_PRESS_CMD),
    PACKET_DST_WATER, offsetof(WATER_DATA, recirculation_enabled),
    RECIRC_STATUS_FLAG_HOTBUTTON_ON, RECIRC_STATUS_FLAG_HOTBUTTON_ON
  ));
}


void NavienLink::send_dhw_set_temp_cmd(float temp){
  uint8_t cmd[sizeof(DHW_SET_TEMP_CMD_TEMPLATE)];
  memcpy(cmd, DHW_SET_TEMP_CMD_TEMPLATE, sizeof(DHW_SET_TEMP_CMD_TEMPLATE));
  uint8_t temp_as_int = temp * 2 + 0.5;
  cmd[9] = temp_as_int;
  cmd[18] = NavienLink::checksum(cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE) - 1, CHECKSUM_SEED_62);

  send_cmd(NavienCmd(
    cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE),
    PACKET_DST_GAS, offsetof(GAS_DATA, dhw_set_temp),
    0xFF, temp_as_int,
    20 // retry longer because sometimes the unit takes longer to respond to this command
  ));
}

void NavienLink::send_scheduled_recirculation_on_cmd(){
  send_cmd(NavienCmd(
    SCHEDULED_RECIRC_ON_CMD, sizeof(SCHEDULED_RECIRC_ON_CMD),
    PACKET_DST_WATER, offsetof(WATER_DATA, recirculation_enabled),
    RECIRC_STATUS_FLAG_SCHEDULED_ON, RECIRC_STATUS_FLAG_SCHEDULED_ON
  ));
}

void NavienLink::send_scheduled_recirculation_off_cmd(){
  send_cmd(NavienCmd(
    SCHEDULED_RECIRC_OFF_CMD, sizeof(SCHEDULED_RECIRC_OFF_CMD),
    PACKET_DST_WATER, offsetof(WATER_DATA, recirculation_enabled),
    RECIRC_STATUS_FLAG_SCHEDULED_ON, 0x00
  ));
}

/**
 * Convert flow units to liters/min values
 * flow is reported as 0.1 liter units.
 */
float NavienLink::flow2lpm(uint8_t f){
  return (float)f / 10.f;
}
  
/**
 * Convert flow units to GPM values
 * flow is reported as 0.1 liter units.
 * we're calculating flow in liters/min and then
 * converting to gallons resulting in GPM (Gallons Per Min)
 */
float NavienLink::flow2gpm(uint8_t f){
  return  (float)f / 10.f / 3.785f;
}

float NavienLink::t2c(uint8_t c){
  return (float)c / 2.f;
}

float NavienLink::ot2c(uint8_t c){
  // Value when no probe is connected.
  // Decodes to right at low end of many 10k thermistors (-30C) so probably min reportable?
  if (c == 0x9E) {
    return NAN;
  }
  float sign = c & 0x80 ? -1 : 1;
  float mag = c & 0x7F;
  return sign * mag;
}
  
uint8_t NavienLink::t2f(uint8_t c){
  float f = c;

  f /= 2;

  f = f * 9.f / 5.f + 32.f;

  return (uint8_t)round(f);
}

  
void NavienLink::print_buffer(const uint8_t *data, size_t length) {
   char hex_buffer[100];
   hex_buffer[(3 * 32) + 1] = 0;
   for (size_t i = 0; i < length; i++) {
     size_t offset = 3 * (i % 32);
     snprintf(&hex_buffer[offset], sizeof(hex_buffer) - offset, "%02X ", data[i]);
     if (i % 32 == 31) {
       ESP_LOGI(TAG, "   %s", hex_buffer);
     }
   }
   if (length % 32) {
     // null terminate if incomplete line
     hex_buffer[3 * (length % 32) + 2] = 0;
     ESP_LOGI(TAG, "   %s", hex_buffer);
   }
 }

  
uint8_t NavienLink::checksum(const uint8_t * buffer, uint8_t len, uint16_t seed){  
  uint16_t result;

  if (len < 2) {
    result = 0x00;
  }
  else {
    result = 0xff;

    for (uint i = 0; i < len; i++){
      result = result << 1;

      if (result > 0xff){
	result = (result & 0xff) ^ seed;
      }

      // this is important!!
      // the checksum is calculated
      // based on the lower byte, i.e.
      // only the lower byte is XOR-ed
      result = ((uint8_t)result) ^ (uint16_t)buffer[i];
    }
  }
  return result;
}


  
}  // namespace navien
}  // namespace esphome
