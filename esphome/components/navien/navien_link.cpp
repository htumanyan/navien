#include <cmath>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"


namespace esphome {
namespace navien {

static const char *TAG = "navien.link";


bool NavienLink::seek_to_marker(){
  uint8_t byte;

  int available = this->uart.available();
  for (int i = 0; i < available; i++){
    this->uart.peek_byte(&byte);
    if (byte == PACKET_MARKER)
      return true;
    
    this->uart.read_byte(&byte);
  }
  return false;
}


void NavienLink::parse_control_packet(){
  ESP_LOGV(TAG, "Got Control Packet => %d bytes", this->recv_buffer.hdr.len + HDR_SIZE);
  //  Navien::print_buffer(this->recv_buffer.raw_data, this->recv_buffer.hdr.len + HDR_SIZE);
}
  
void NavienLink::parse_status_packet(){
  switch(this->recv_buffer.hdr.packet_type){
  case PACKET_TYPE_WATER:
    //NavienLink::print_buffer(this->recv_buffer.raw_data, this->recv_buffer.hdr.len + HDR_SIZE);
    ESP_LOGV(TAG, "B8: 0x%02X, B32: 0x%02X, r_enabled: 0x%02X",
	     this->recv_buffer.water.unknown_06,
	     this->recv_buffer.water.unknown_32,
	     this->recv_buffer.water.recirculation_enabled);
    this->cb.on_water(recv_buffer.water);
    break;
  case PACKET_TYPE_GAS:
    this->cb.on_gas(recv_buffer.gas);
    break;
  }
}
  
void NavienLink::parse_packet(){
  uint8_t crc_c = 0x00;
  uint8_t crc_r = 0x00;

  //NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len + 1);
  crc_r = this->recv_buffer.raw_data[HDR_SIZE + this->recv_buffer.hdr.len];
  
  switch(this->recv_buffer.hdr.direction){
  case PACKET_DIRECTION_STATUS:
    crc_c = NavienLink::checksum(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len, CHECKSUM_SEED_4B);
    if (crc_c != crc_r){
      ESP_LOGE(TAG, "Status Packet checksum error: 0x%02X (calc) != 0x%02X (recv)", crc_c, crc_r);
      NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len + 1);
      break;
    }
    parse_status_packet();
    break;
  case PACKET_DIRECTION_CONTROL:
    crc_c = NavienLink::checksum(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len, CHECKSUM_SEED_62);
    if (crc_c != crc_r){
      ESP_LOGE(TAG, "Control Packet checksum error: 0x%02X (calc) != 0x%02X (recv)", crc_c, crc_r);
      NavienLink::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len + 1);
      break;
    }
    parse_control_packet();
    break;
  }

  //  ESP_LOGV(TAG, "Calculated checksum over %d bytes => 0x%02X", HDR_SIZE + this->recv_buffer.hdr.len, crc);

}

  
void NavienLink::receive() {
  uint8_t byte;

  int available = this->uart.available();

  if (!available)
    return;

  ESP_LOGV(TAG, "%d bytes available", available);
  while(available){
    switch(this->recv_state){
    case INITIAL:
      if (this->seek_to_marker()){
	this->recv_state = MARKER_FOUND;
	ESP_LOGV(TAG, "Marker Found");
	break;
      }
      // No marker found and no data left to read. Exit and wait
      // for more bytes to come.
      return;
    case MARKER_FOUND:
      available = this->uart.available();
      if (available < HDR_SIZE){
	ESP_LOGV(TAG, "Only %d bytes available - less than header size", available);
	return;
      }
      if (!this->uart.read_array(this->recv_buffer.raw_data, HDR_SIZE)){
	ESP_LOGV(TAG, "Failed to read header");
	break;
      }
      this->recv_state = HEADER_PARSED;
      ESP_LOGV(TAG, "Parsed header. %d bytes of body left", this->recv_buffer.hdr.len);
      
    case HEADER_PARSED:
      available = this->uart.available();

      // +1 here is for the checksum - it is in the last byte
      uint8_t len = this->recv_buffer.hdr.len + 1;
      if (available < len){
	ESP_LOGV(TAG, "Only %d data bytes available - less than len", available);
	return;
      }
      if (!this->uart.read_array(this->recv_buffer.raw_data + HDR_SIZE, len)){
	ESP_LOGV(TAG, "Failed to read %d bytes", len);
	break;
      }
      ESP_LOGV(TAG, "Got Packet => %d bytes", len+HDR_SIZE );

      if (!this->cmd_buffer.empty()){
	/// there are queued commands. Need to send them
	NAVIEN_CMD cmd = cmd_buffer.back();
	cmd_buffer.pop_back();
	this->uart.write_array(cmd.buffer, cmd.len);
      }else{
	//Navilink keeps sending this sequence every time it receives a packet
	//this->send_cmd(NAVILINK_PRESENT, sizeof(NAVILINK_PRESENT));
      }
       
      //Navien::print_buffer(this->recv_buffer.raw_data, len+HDR_SIZE);
      this->parse_packet();
      available = this->uart.available();
      this->recv_state = INITIAL;
    }
  }
}

void NavienLink::send_cmd(const uint8_t * buffer, uint8_t len){
  /*  if (buffer && len){
    this->write_array(buffer, len);
    }*/
  this->cmd_buffer.push_front(NAVIEN_CMD(buffer, len));
}
  
void NavienLink::send_turn_on_cmd(){
  // Send twice. In experiments I've noticed
  // that sending once does not always work and that
  // the Navilink sends the commands multiple times
  this->send_cmd(TURN_ON_CMD, sizeof(TURN_ON_CMD));
  this->send_cmd(TURN_ON_CMD, sizeof(TURN_ON_CMD));
}

void NavienLink::send_turn_off_cmd(){
  // Send twice. In experiments I've noticed
  // that sending once does not always work and that
  // the Navilink sends the commands multiple times
  this->send_cmd(TURN_OFF_CMD, sizeof(TURN_OFF_CMD));
  this->send_cmd(TURN_OFF_CMD, sizeof(TURN_OFF_CMD));
}

void NavienLink::send_hot_button_cmd(){
  //this->send_cmd(RECIRC_ON_CMD, sizeof(RECIRC_ON_CMD));
  //return;  
    this->send_cmd(HOT_BUTTON_PRESS_CMD, sizeof(HOT_BUTTON_PRESS_CMD));
    //    delay(1);
    // this->send_cmd(HOT_BUTTON_PRESS_CMD, sizeof(HOT_BUTTON_PRESS_CMD));
    //delay(15);
    this->send_cmd(HOT_BUTTON_RELSE_CMD, sizeof(HOT_BUTTON_RELSE_CMD));
}
  

void NavienLink::send_set_temp_cmd(float temp){
  uint8_t cmd[19];
  memcpy(cmd, SET_TEMP_CMD_TEMPLATE, sizeof(SET_TEMP_CMD_TEMPLATE));
  cmd[9] = temp * 2;
  cmd[18] = NavienLink::checksum(cmd, sizeof(SET_TEMP_CMD_TEMPLATE) - 1, CHECKSUM_SEED_62);

  NavienLink::print_buffer(cmd, sizeof(SET_TEMP_CMD_TEMPLATE));
  this->send_cmd(cmd, sizeof(SET_TEMP_CMD_TEMPLATE));
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

uint8_t NavienLink::t2c(uint8_t c){
  return (float)c / 2.f;
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
     snprintf(&hex_buffer[3 * (i % 32)], sizeof(hex_buffer), "%02X ", data[i]);
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
