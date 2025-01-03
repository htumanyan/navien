#include <cmath>
#include "esphome/core/log.h"
#include "navien.h"


namespace esphome {
namespace navien {

static const char *TAG = "navien.sensor";

void Navien::setup() {
}

void Navien::update() {
  if (this->target_temp_sensor != nullptr)
    this->target_temp_sensor->publish_state(this->state.gas.set_temp);

  if (this->outlet_temp_sensor != nullptr)
    this->outlet_temp_sensor->publish_state(this->state.gas.outlet_temp);

  if (this->inlet_temp_sensor != nullptr)
      this->inlet_temp_sensor->publish_state(this->state.gas.inlet_temp);

    if (this->water_flow_sensor != nullptr)
      this->water_flow_sensor->publish_state(this->state.water.flow_lpm);
}
  /*
void Navien::clear_buffer(){
  //  Navien::print_buffer(this->recv_buffer.raw_data, this->recv_ptr);
  memset(this->recv_buffer.raw_data, 0x00, sizeof(this->recv_buffer.raw_data));
  }*/

bool Navien::seek_to_marker(){
  uint8_t byte;

  int available = this->available();
  for (int i = 0; i < available; i++){
    this->peek_byte(&byte);
    if (byte == PACKET_MARKER)
      return true;
    
    this->read_byte(&byte);
  }
  return false;
}

void Navien::parse_water(){
  ESP_LOGV(TAG, "Got Water Packet => %d bytes", this->recv_buffer.hdr.len + HDR_SIZE);
  //Navien::print_buffer(this->recv_buffer.raw_data, this->recv_buffer.hdr.len + HDR_SIZE);
      
  ESP_LOGV(TAG, "Received Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, Flow: 0x%02X",
	   this->recv_buffer.water.set_temp,
	   this->recv_buffer.water.inlet_temp,
	   this->recv_buffer.water.outlet_temp,
	   this->recv_buffer.water.water_flow
  
  );
  /*
  this->state.water.set_temp    = Navien::t2f(this->recv_buffer.water.set_temp);
  this->state.water.outlet_temp = Navien::t2f(this->recv_buffer.water.outlet_temp);
  this->state.water.inlet_temp = Navien::t2f(this->recv_buffer.water.inlet_temp);
  this->state.water.flow_gpm = Navien::flow2gpm(this->recv_buffer.water.water_flow);
  */
  
  this->state.water.set_temp    = Navien::t2c(this->recv_buffer.water.set_temp);
  this->state.water.outlet_temp = Navien::t2c(this->recv_buffer.water.outlet_temp);
  this->state.water.inlet_temp = Navien::t2c(this->recv_buffer.water.inlet_temp);

  
  this->state.water.flow_lpm = Navien::flow2lpm(this->recv_buffer.water.water_flow);

  ESP_LOGV(TAG, "Set Temp: %d, Inlet: %d, Outlet: %d, flow: %f",
	   this->state.water.set_temp,
	   this->state.water.inlet_temp,
	   this->state.water.outlet_temp,
	   this->state.water.flow_lpm
  );
}

void Navien::parse_gas(){
  //Navien::print_buffer(this->recv_buffer.raw_data, this->recv_buffer.hdr.len + HDR_SIZE);
  ESP_LOGV(TAG, "Received Gas Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X",
	   this->recv_buffer.gas.set_temp,
	   this->recv_buffer.gas.inlet_temp,
	   this->recv_buffer.gas.outlet_temp
  );
  
  this->state.gas.set_temp    = Navien::t2c(this->recv_buffer.gas.set_temp);
  this->state.gas.outlet_temp = Navien::t2c(this->recv_buffer.gas.outlet_temp);
  this->state.gas.inlet_temp = Navien::t2c(this->recv_buffer.gas.inlet_temp);
  
  this->state.gas.controller_version = this->recv_buffer.gas.controller_version_hi << 8 | this->recv_buffer.gas.controller_version_lo;
  this->state.gas.panel_version = this->recv_buffer.gas.panel_version_hi << 8 | this->recv_buffer.gas.panel_version_lo;
  this->state.gas.accumulated_gas_usage = this->recv_buffer.gas.cumulative_gas_hi << 8 | this->recv_buffer.gas.cumulative_gas_lo;
  this->state.gas.current_gas_usage = this->recv_buffer.gas.current_gas_hi << 8 | this->recv_buffer.gas.current_gas_lo;
  /*
  this->state.gas.set_temp    = Navien::t2f(this->recv_buffer.gas.set_temp);
  this->state.gas.outlet_temp = Navien::t2f(this->recv_buffer.gas.outlet_temp);
  this->state.gas.inlet_temp = Navien::t2f(this->recv_buffer.gas.inlet_temp);
  */

  ESP_LOGV(TAG, "Set Gas Temp: %d, Inlet: %d, Outlet: %d, Controller: %d, Panel: %d, Cur Usage: %d, Accum Usage %d",
	   this->state.gas.set_temp,
	   this->state.gas.inlet_temp,
	   this->state.gas.outlet_temp,
	   this->state.gas.controller_version,
	   this->state.gas.panel_version,
	   this->state.gas.current_gas_usage,
	   this->state.gas.accumulated_gas_usage
	   );
}

void Navien::parse_control_packet(){
  ESP_LOGV(TAG, "Got Control Packet => %d bytes", this->recv_buffer.hdr.len + HDR_SIZE);
  //  Navien::print_buffer(this->recv_buffer.raw_data, this->recv_buffer.hdr.len + HDR_SIZE);
}
  
void Navien::parse_status_packet(){
  switch(this->recv_buffer.hdr.packet_type){
  case PACKET_TYPE_WATER:
    parse_water();
    break;
  case PACKET_TYPE_GAS:
    parse_gas();
    break;
  }
}
  
void Navien::parse_packet(){
  uint8_t crc_c = 0x00;
  uint8_t crc_r = 0x00;

  //Navien::print_buffer(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len + 1);
  crc_r = this->recv_buffer.raw_data[HDR_SIZE + this->recv_buffer.hdr.len];
  
  switch(this->recv_buffer.hdr.direction){
  case PACKET_DIRECTION_STATUS:
    crc_c = Navien::checksum(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len, CHECKSUM_SEED_4B);
    if (crc_c != crc_r){
      ESP_LOGE(TAG, "Status Packet checksum error: 0x%02X (calc) != 0x%02X (recv)", crc_c, crc_r);
      break;
    }
    parse_status_packet();
    break;
  case PACKET_DIRECTION_CONTROL:
    crc_c = Navien::checksum(this->recv_buffer.raw_data, HDR_SIZE + this->recv_buffer.hdr.len, CHECKSUM_SEED_62);
    if (crc_c != crc_r){
      ESP_LOGE(TAG, "Control Packet checksum error: 0x%02X (calc) != 0x%02X (recv)", crc_c, crc_r);
      break;
    }
    parse_control_packet();
    break;
  }

  ESP_LOGV(TAG, "Calculated checksum over %d bytes => 0x%02X", HDR_SIZE + this->recv_buffer.hdr.len, crc);

}

  
void Navien::loop() {
  uint8_t byte;

  int available = this->available();

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
      available = this->available();
      if (available < HDR_SIZE){
	ESP_LOGV(TAG, "Only %d bytes available - less than header size", available);
	return;
      }
      if (!read_array(this->recv_buffer.raw_data, HDR_SIZE)){
	ESP_LOGV(TAG, "Failed to read header");
	break;
      }
      this->recv_state = HEADER_PARSED;
      ESP_LOGV(TAG, "Parsed header. %d bytes of body left", this->recv_buffer.hdr.len);
      
    case HEADER_PARSED:
      available = this->available();

      // +1 here is for the checksum - it is in the last byte
      uint8_t len = this->recv_buffer.hdr.len + 1;
      if (available < len){
	ESP_LOGV(TAG, "Only %d data bytes available - less than len", available);
	return;
      }
      if (!read_array(this->recv_buffer.raw_data + HDR_SIZE, len)){
	ESP_LOGV(TAG, "Failed to read %d bytes", len);
	break;
      }
      ESP_LOGV(TAG, "Got Packet => %d bytes", len+HDR_SIZE );
      //Navien::print_buffer(this->recv_buffer.raw_data, len+HDR_SIZE);
      this->parse_packet();
      available = this->available();
      this->recv_state = INITIAL;
    }
  }
}

void Navien::dump_config(){
    ESP_LOGCONFIG(TAG, "Empty UART sensor");
}

 /**
 * Convert flow units to liters/min values
 * flow is reported as 0.1 liter units.
 */
float Navien::flow2lpm(uint8_t f){
  return (float)f / 10.f;
}
  
/**
 * Convert flow units to GPM values
 * flow is reported as 0.1 liter units.
 * we're calculating flow in liters/min and then
 * converting to gallons resulting in GPM (Gallons Per Min)
 */
float Navien::flow2gpm(uint8_t f){
  return  (float)f / 10.f / 3.785f;
}

uint8_t Navien::t2c(uint8_t c){
  return (float)c / 2.f;
}
  
uint8_t Navien::t2f(uint8_t c){
  float f = c;

  f /= 2;

  f = f * 9.f / 5.f + 32.f;

  return (uint8_t)round(f);
}
  
void Navien::print_buffer(const uint8_t *data, size_t length) {
   char hex_buffer[100];
   hex_buffer[(3 * 32) + 1] = 0;
   for (size_t i = 0; i < length; i++) {
     snprintf(&hex_buffer[3 * (i % 32)], sizeof(hex_buffer), "%02X ", data[i]);
     if (i % 32 == 31) {
       ESP_LOGD(TAG, "   %s", hex_buffer);
     }
   }
   if (length % 32) {
     // null terminate if incomplete line
     hex_buffer[3 * (length % 32) + 2] = 0;
     ESP_LOGD(TAG, "   %s", hex_buffer);
   }
 }

  
uint8_t Navien::checksum(const uint8_t * buffer, uint8_t len, uint16_t seed){  
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
