#include <cmath>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"


namespace esphome {
namespace navien {

static const char *TAG = "navien.link";
// Raw capture lines get their own tag, so they can be filtered separately,
// e.g. "logger: logs: navien.raw: INFO" hides them on a DEBUG build.
static const char *RAW_TAG = "navien.raw";

bool NavienLink::raw_log_enabled_() const {
#if defined(ESPHOME_LOG_LEVEL) && ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_DEBUG
  return true;
#else
  return this->raw_capture_;
#endif
}

/**
 * One line per frame, e.g.
 *   [I][navien.raw]: RX 123456: F7 05 50 50 90 22 42 00 ... 87
 * The number is millis(), so gaps between frames can be measured.
 */
void NavienLink::log_raw_(const char *what, const uint8_t *data, size_t len) {
  if (!this->raw_log_enabled_())
    return;
  if (data != this->junk_)
    this->flush_junk_();

  static const char HEX_DIGITS[] = "0123456789ABCDEF";
  char hex[3 * sizeof(RECV_BUFFER) + 1];
  const size_t n = std::min(len, sizeof(RECV_BUFFER));
  for (size_t i = 0; i < n; i++) {
    hex[3 * i] = HEX_DIGITS[data[i] >> 4];
    hex[3 * i + 1] = HEX_DIGITS[data[i] & 0x0F];
    hex[3 * i + 2] = ' ';
  }
  hex[n ? 3 * n - 1 : 0] = '\0';

  if (this->raw_capture_) {
    ESP_LOGI(RAW_TAG, "%s %u: %s", what, (unsigned) millis(), hex);
  } else {
    ESP_LOGD(RAW_TAG, "%s %u: %s", what, (unsigned) millis(), hex);
  }
}

void NavienLink::note_junk_(uint8_t byte) {
  if (!this->raw_log_enabled_())
    return;
  this->junk_[this->junk_len_++] = byte;
  if (this->junk_len_ == sizeof(this->junk_))
    this->flush_junk_();
}

void NavienLink::flush_junk_() {
  if (this->junk_len_ == 0)
    return;
  const uint8_t len = this->junk_len_;
  this->junk_len_ = 0;
  this->log_raw_("RX (between frames)", this->junk_, len);
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
    if (byte == PACKET_MARKER) {
      this->flush_junk_();
      return true;
    }

    uart->read_byte(&byte);
    this->note_junk_(byte);
  }
  this->flush_junk_();
  return false;
}


bool NavienLink::should_log_(uint32_t &last_ms, bool &logged_once){
  uint32_t now = millis();
  if (logged_once && now - last_ms < LOG_THROTTLE_MS)
    return false;
  last_ms = now;
  logged_once = true;
  return true;
}

/**
 * A packet another controller (e.g. a real NaviLink) sends to the unit:
 * F7 xx 0F 50 10 ... The system type byte (xx) is ignored because it follows
 * the unit's model rather than identifying the sender.
 */
bool NavienLink::is_from_other_controller_() const {
  const HEADER &hdr = this->recv_buffer.hdr;
  return hdr.packet_marker == PACKET_MARKER && hdr.src == PACKET_SRC_CONTROL &&
         hdr.dst == PACKET_DST_WATER && hdr.direction == PACKET_DIR_CONTROL;
}

bool NavienLink::is_navilink_present_() const {
  const uint8_t body_offset = HDR_SIZE;
  const uint8_t body_len = NAVILINK_PRESENT[offsetof(HEADER, len)];
  return this->is_from_other_controller_() && this->recv_buffer.hdr.len == body_len &&
         std::memcmp(this->recv_buffer.raw_data + body_offset, NAVILINK_PRESENT + body_offset, body_len) == 0;
}

void NavienLink::parse_control_packet(){
  ESP_LOGV(TAG, "Got Control Packet => %d bytes", this->recv_buffer.hdr.len + HDR_SIZE);
  if (!this->other_navilink_installed && this->is_navilink_present_()){
    /* This is a NAVILINK_PRESENT_PKT that wasn't sent by us, so anothe NaviLink is also hooked up */
    ESP_LOGW(TAG, "Detected NAVILINK_PRESENT packet from another NaviLink device, will stop sending NAVILINK_PRESENT packets until rebooted");
    this->other_navilink_installed = true;
  }
  //  Navien::print_buffer(this->recv_buffer.raw_data, this->recv_buffer.hdr.len + HDR_SIZE);
}

void NavienLink::parse_status_packet(){
  const HEADER &hdr = this->recv_buffer.hdr;

  // Track the unit's system type so our commands can use one it accepts.
  if (hdr.sys_type != this->unit_sys_type_) {
    this->unit_sys_type_ = hdr.sys_type;
    ESP_LOGI(TAG, "Unit reports system type 0x%02X (SRC:0x%02X); sending commands with system type 0x%02X",
             hdr.sys_type, hdr.src, this->command_sys_type());
  }

  uint8_t expected_len = 0;
  switch (hdr.dst) {
  case PACKET_DST_WATER:
    expected_len = sizeof(WATER_DATA);
    break;
  case PACKET_DST_GAS:
    expected_len = sizeof(GAS_DATA);
    break;
  default:
    // Not a packet type we decode. Log it so new models can be mapped.
    if (this->should_log_(this->last_unknown_log_ms_, this->unknown_logged_)) {
      ESP_LOGD(TAG, "Unhandled status packet SRC:0x%02X DST:0x%02X type 0x%02X len %u:",
               hdr.src, hdr.dst, hdr.sys_type, hdr.len);
      NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + hdr.len + 1);
    }
    return;
  }

  if (hdr.len < expected_len) {
    // Missing trailing fields read as zero: receive() cleared the buffer tail
    // past the checksum, and the checksum byte (already verified) is cleared
    // here.
    bool &logged = hdr.dst == PACKET_DST_WATER ? this->short_water_logged_ : this->short_gas_logged_;
    if (!logged) {
      logged = true;
      ESP_LOGI(TAG, "Status packet SRC:0x%02X DST:0x%02X is %u bytes, expected %u; missing fields read as 0:",
               hdr.src, hdr.dst, hdr.len, expected_len);
      NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + hdr.len + 1);
    }
    this->recv_buffer.raw_data[HDR_SIZE + hdr.len] = 0;
  }

  switch(hdr.dst){
  case PACKET_DST_WATER:
    ESP_LOGD(TAG, "SRC:0x%02X B8: 0x%02X, B32: 0x%02X, r_enabled: 0x%02X",
             hdr.src,
             this->recv_buffer.water.unknown_06,
             this->recv_buffer.water.unknown_32,
             this->recv_buffer.water.recirculation_enabled);
    for (uint8_t i = 0; i < NAVIEN_CASCADE_MAX; ++i)
      if (visitors_[i]) visitors_[i]->on_water(recv_buffer.water, hdr.src);
    break;
  case PACKET_DST_GAS:
    ESP_LOGD(TAG, "SRC:0x%02X => Gas", hdr.src);
    for (uint8_t i = 0; i < NAVIEN_CASCADE_MAX; ++i)
      if (visitors_[i]) visitors_[i]->on_gas(recv_buffer.gas, hdr.src);
    break;
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

  const uint32_t now = millis();
  this->prev_poll_ms_ = this->poll_ms_ != 0 ? this->poll_ms_ : now;  // first call: no history
  this->poll_ms_ = now;

  int available = uart->available();
  if (!available) {
    // A frame's bytes arrive back to back. If the rest of one hasn't shown up
    // after this long, its header was noise: drop it, or we'd stay stuck
    // mid-frame (and never transmit) until the unit's next packets arrive
    // and get swallowed as its body.
    if (this->recv_state != INITIAL && millis() - this->last_rx_ms_ >= FRAME_TIMEOUT_MS) {
      ESP_LOGD(TAG, "Dropping incomplete frame after %u ms of silence", (unsigned) FRAME_TIMEOUT_MS);
      if (this->recv_state == HEADER_PARSED)
        this->log_raw_("RX (incomplete, header only)", this->recv_buffer.raw_data, HDR_SIZE);
      this->recv_state = INITIAL;
    }
    // The bus is idle right now - the only safe moment to talk.
    this->transmit_if_quiet_();
    return;
  }
  this->last_rx_ms_ = millis();

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
        // The length byte is untrusted: a corrupted header must not make us
        // read past recv_buffer. Drop the frame and resync on the next marker.
        if (this->recv_buffer.hdr.len > MAX_BODY_LEN) {
          this->log_raw_("RX (bad length, header only)", this->recv_buffer.raw_data, HDR_SIZE);
          if (this->should_log_(this->last_bad_len_log_ms_, this->bad_len_logged_)) {
            ESP_LOGW(TAG, "Dropping frame with length %u (max %u):", this->recv_buffer.hdr.len, MAX_BODY_LEN);
            NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE);
          }
          this->recv_state = INITIAL;
          available = uart->available();
          break;
        }

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
        // Clear what's left of the previous packet, so a packet shorter than
        // the struct we decode it with reads zeros instead of stale data.
        std::memset(this->recv_buffer.raw_data + HDR_SIZE + len, 0, sizeof(this->recv_buffer.raw_data) - HDR_SIZE - len);
        ESP_LOGV(TAG, "Got Packet => %d bytes", len + HDR_SIZE);
        this->log_raw_("RX", this->recv_buffer.raw_data, HDR_SIZE + len);

        // Don't write here: the unit's next packet may already be arriving.
        // Arm a reply; transmit_if_quiet_() sends it once the bus goes quiet.
        this->last_rx_ms_ = millis();
        this->last_frame_ms_ = this->prev_poll_ms_;
        this->arm_reply_();

        // Navien::print_buffer(this->recv_buffer.raw_data, len + HDR_SIZE);
        this->parse_packet();
        available = uart->available();
        this->recv_state = INITIAL;
        break;
      }
    }
  }
}

/**
 * Called at the end of every received packet: owe the bus a reply, sent by
 * transmit_if_quiet_() once it goes quiet.
 */
void NavienLink::arm_reply_(){
  const bool from_other_controller = this->is_from_other_controller_();
  // Remember whether another NaviLink spoke in this burst, even if the unit
  // talks after it: that burst's quiet gap is where our commands may go.
  this->tx_after_other_controller_ = this->tx_after_other_controller_ || from_other_controller;

  if (!this->tx_armed_) {
    this->tx_armed_ = true;
    this->tx_armed_ms_ = millis();
    return;
  }

  // The bus hasn't been quiet since the reply was armed. Rather than never
  // talking, fall back to replying straight after a packet (the behaviour
  // before quiet-bus transmit), with the old rule for sharing the bus.
  if (millis() - this->tx_armed_ms_ >= TX_FALLBACK_MS &&
      (!this->other_navilink_installed || from_other_controller)) {
    this->send_reply_(from_other_controller);
  }
}

void NavienLink::transmit_if_quiet_(){
  if (!this->tx_armed_ || this->recv_state != INITIAL)
    return;
  const uint32_t now = millis();
  if (now - this->last_rx_ms_ < TX_QUIET_MS)
    return;

  // Too late for this slot (see TX_LATE_MS): skip it and reply after the
  // unit's next packet. A queued command stays queued. If every slot comes out
  // late, reply anyway now and then, so the unit still hears from us.
  if (now - this->last_frame_ms_ > TX_LATE_MS && this->late_skips_ < TX_MAX_LATE_SKIPS) {
    this->late_skips_++;
    this->tx_armed_ = false;
    this->tx_after_other_controller_ = false;
    this->log_raw_("TX skipped (late)", nullptr, 0);
    return;
  }
  this->late_skips_ = 0;
  this->send_reply_(this->tx_after_other_controller_);
}

/**
 * The reply that used to be written straight after each packet. Same
 * decisions as before - a queued command if there is one (and either no other
 * NaviLink, or it's that NaviLink's slot), else our own NAVILINK_PRESENT -
 * only the timing changed (see transmit_if_quiet_()).
 */
void NavienLink::send_reply_(bool in_other_controller_slot){
  this->tx_armed_ = false;
  this->tx_after_other_controller_ = false;

  if (!this->cmd_buffer.empty()) {
    if (!this->other_navilink_installed || in_other_controller_slot) {
      NAVIEN_CMD cmd = cmd_buffer.back();
      cmd_buffer.pop_back();
      this->write_packet_(cmd.buffer, cmd.len);
    }
  } else if (!this->other_navilink_installed) {
    // No pending command: send NAVILINK_PRESENT so the unit knows we're here.
    // When the unit is in an automatic recirculation mode, this tells it we're
    // controlling when it does and does not recirculate (and it triggers the
    // "Recirculation settings must be configured through the NaviLink app"
    // message on the unit's front panel).
    this->write_packet_(NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT));
  }
}

/**
 * Pick the system type byte for our control packets.
 *
 * Units that report 0x05 (NPE) or 0x06 (NCB-H) get 0x05, which both are known
 * to accept. Any other reported value is mirrored back: the NHB-H (0x04)
 * ignores 0x05 and only accepts its own value (upstream PR #91). Models that
 * haven't been captured yet (e.g. NFB) are treated the same way. The NFC-250
 * reports 0x06 and accepts 0x05 (verified).
 * Before the first status packet the existing 0x05 is used.
 */
uint8_t NavienLink::command_sys_type() const {
  if (this->command_sys_type_override_ != 0)
    return this->command_sys_type_override_;

  switch (this->unit_sys_type_) {
  case SYS_TYPE_UNKNOWN:
  case SYS_TYPE_NPE:
  case SYS_TYPE_NCB_H:
    return SYS_TYPE_NPE;
  default:
    return this->unit_sys_type_;
  }
}

void NavienLink::write_packet_(const uint8_t *data, uint8_t len){
  const uint8_t sys_type = this->command_sys_type();
  uint8_t buf[sizeof(NAVIEN_CMD::buffer)];

  if (len <= HDR_SIZE || len > sizeof(buf) || data[offsetof(HEADER, sys_type)] == sys_type) {
    this->log_raw_("TX", data, len);
    uart->write_array(data, len);
    return;
  }

  // Same packet with the unit's system type; the checksum covers that byte.
  std::memcpy(buf, data, len);
  buf[offsetof(HEADER, sys_type)] = sys_type;
  buf[len - 1] = NavienLink::checksum(buf, len - 1, CHECKSUM_SEED_62);
  this->log_raw_("TX", buf, len);
  uart->write_array(buf, len);
}

void NavienLink::send_cmd(const uint8_t * buffer, uint8_t len, uint8_t tries){
  // Send multiple times by default. In experiments I've noticed
  // that sending once does not always work and that
  // the NaviLink sends the commands multiple times
  for (uint8_t i = 0; i < tries; i++) {
    this->cmd_buffer.push_front(NAVIEN_CMD(buffer, len));
  }
}
  
void NavienLink::send_turn_on_cmd(){
  this->send_cmd(TURN_ON_CMD, sizeof(TURN_ON_CMD));
}

void NavienLink::send_turn_off_cmd(){
  this->send_cmd(TURN_OFF_CMD, sizeof(TURN_OFF_CMD));
}

void NavienLink::send_hot_button_cmd(){
  this->send_cmd(HOT_BUTTON_PRESS_CMD, sizeof(HOT_BUTTON_PRESS_CMD));
  this->send_cmd(HOT_BUTTON_RELSE_CMD, sizeof(HOT_BUTTON_RELSE_CMD), 1);
}
  

/**
 * Setpoints travel as deg C * 2 in one byte, where 0 means "no change".
 * Reject anything that can't be encoded (or makes no sense for water)
 * instead of letting the float-to-byte conversion wrap or misbehave.
 */
static bool valid_setpoint(float temp){
  if (!std::isfinite(temp) || temp < 1.0f || temp > 100.0f) {
    ESP_LOGW(TAG, "Ignoring setpoint %.1f C - outside 1..100 C", temp);
    return false;
  }
  return true;
}

void NavienLink::send_dhw_set_temp_cmd(float temp){
  if (!valid_setpoint(temp))
    return;
  uint8_t cmd[19];
  memcpy(cmd, DHW_SET_TEMP_CMD_TEMPLATE, sizeof(DHW_SET_TEMP_CMD_TEMPLATE));
  cmd[9] = temp * 2 + 0.5;
  cmd[18] = NavienLink::checksum(cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE) - 1, CHECKSUM_SEED_62);

  NavienLink::print_buffer(cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE));
  this->send_cmd(cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE));
}

/**
 * Space-heating setpoint (combi / boiler units).
 *
 * Same 0x4F control packet as the DHW setpoint, with the temperature in
 * byte 10 instead of byte 9 (deg C * 2). Byte 9 stays 0x00, which the unit
 * treats as "leave DHW unchanged" - the mirror of DHW commands carrying 0x00
 * in byte 10, which never move the SH setpoint.
 *
 * Verified on an NCB-240/110H (device type NCB_H, f7 06 framing): commands
 * for 160.7 F, 158.9 F and 156.2 F were each followed by the unit reporting
 * that value in the gas packet's sh_set_temp. Byte 10 is the slot tankless
 * units always leave 0x00.
 */
void NavienLink::send_sh_set_temp_cmd(float temp){
  if (!valid_setpoint(temp))
    return;
  uint8_t cmd[19];
  memcpy(cmd, DHW_SET_TEMP_CMD_TEMPLATE, sizeof(DHW_SET_TEMP_CMD_TEMPLATE));
  cmd[10] = temp * 2 + 0.5;
  cmd[18] = NavienLink::checksum(cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE) - 1, CHECKSUM_SEED_62);

  NavienLink::print_buffer(cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE));
  this->send_cmd(cmd, sizeof(DHW_SET_TEMP_CMD_TEMPLATE));
}

void NavienLink::send_scheduled_recirculation_on_cmd(){
  this->send_cmd(SCHEDULED_RECIRC_ON_CMD, sizeof(SCHEDULED_RECIRC_ON_CMD));
}

void NavienLink::send_scheduled_recirculation_off_cmd(){
  this->send_cmd(SCHEDULED_RECIRC_OFF_CMD, sizeof(SCHEDULED_RECIRC_OFF_CMD));
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
