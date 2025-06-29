#include <cmath>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"


namespace esphome {
namespace navien {

static const char *TAG = "navien.sensor";
  
void Navien::setup() {
  ESP_LOGCONFIG(TAG, "Setting rs485 into receive mode");
  // Set the rs485 into receive mode
  //pinMode(D5, OUTPUT);
  //digitalWrite(D5, LOW);

  // Set the driver into regular mode (from undefined startup)
  ESP_LOGCONFIG(TAG, "Activating TTL/CMOS buffer");
  //  pinMode(D6, OUTPUT);
  //digitalWrite(D6, HIGH);


  // pinMode(D2, OUTPUT);
  // digitalWrite(D2, HIGH);

  this->state.power = POWER_OFF;
}

void Navien::on_water(const WATER_DATA & water){
    ESP_LOGI(TAG, "Received Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, Flow: 0x%02X, Sys Power: 0x%02X, Sys Status: 0x%02X, Recirc Enabled: 0x%02X",
	   water.set_temp,
	   water.inlet_temp,
	   water.outlet_temp,
	   water.water_flow,
	   water.system_power,
	   water.system_status,
	   water.recirculation_enabled);

    if (water.system_power & POWER_STATUS_ON_OFF_MASK){
      state.power = POWER_ON;
    }else{
      state.power = POWER_OFF;
    }
        
    // Update the counter that will be used in assessment
    // of whether we're connected to navien or not
    this->received_cnt++;
    
    this->state.water.set_temp    = NavienLink::t2f(water.set_temp);
    this->state.water.outlet_temp = NavienLink::t2f(water.outlet_temp);
    this->state.water.inlet_temp = NavienLink::t2f(water.inlet_temp);
    this->state.water.flow_lpm = NavienLink::flow2lpm(water.water_flow);
    this->state.water.utilization = water.operating_capacity * 0.5f;
}
  
void Navien::on_gas(const GAS_DATA & gas){
  ESP_LOGV(TAG, "Received Gas Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X",
	   gas.set_temp,
	   gas.inlet_temp,
	   gas.outlet_temp
  );

  ESP_LOGV(TAG, "Received Accumulated: 0x%02X 0x%02X, Current Gas: 0x%02X 0x%02X",
	   gas.cumulative_gas_hi,
	   gas.cumulative_gas_lo,
	   gas.current_gas_hi,
	   gas.current_gas_lo
  );

  // Update the counter that will be used in assessment
  // of whether we're connected to navien or not
  this->received_cnt++;
    
  this->state.gas.set_temp    = NavienLink::t2c(gas.set_temp);
  this->state.gas.outlet_temp = NavienLink::t2c(gas.outlet_temp);
  this->state.gas.inlet_temp = NavienLink::t2c(gas.inlet_temp);
  
  this->state.gas.accumulated_gas_usage = gas.cumulative_gas_hi << 8 | gas.cumulative_gas_lo;
  this->state.gas.current_gas_usage = gas.current_gas_hi << 8 | gas.current_gas_lo;

  this->state.controller_version = gas.controller_version_hi << 8 | gas.controller_version_lo;
  this->state.panel_version = gas.panel_version_hi << 8 | gas.panel_version_lo;
}
  
void Navien::on_error(){
  this->target_temp_sensor->publish_state(0);
  this->outlet_temp_sensor->publish_state(0);
  this->inlet_temp_sensor->publish_state(0);
  this->water_flow_sensor->publish_state(0);

  this->is_connected = false;
}

void Navien::update_water_sensors(){
  if (this->water_flow_sensor != nullptr)
    this->water_flow_sensor->publish_state(this->state.water.flow_lpm);

  if (this->water_utilization_sensor != nullptr)
    this->water_utilization_sensor->publish_state(this->state.water.utilization);

  #ifdef USE_SWITCH
  if (this->power_switch != nullptr){
    switch(this->state.power){
    case POWER_ON:
      this->power_switch->publish_state(true);
      break;
    default:
      this->power_switch->publish_state(false);
    }
  }
#endif
}

void Navien::update_gas_sensors(){
  if (this->target_temp_sensor != nullptr)
    this->target_temp_sensor->publish_state(this->state.gas.set_temp);
  
  if (this->outlet_temp_sensor != nullptr)
    this->outlet_temp_sensor->publish_state(this->state.gas.outlet_temp);

  if (this->inlet_temp_sensor != nullptr)
      this->inlet_temp_sensor->publish_state(this->state.gas.inlet_temp);

  if (this->gas_total_sensor != nullptr)
    this->gas_total_sensor->publish_state(this->state.gas.accumulated_gas_usage);

  if (this->gas_current_sensor != nullptr)
    this->gas_current_sensor->publish_state(this->state.gas.current_gas_usage);
}
  
void Navien::update() {
  ESP_LOGV(TAG, "Conn Status: received: %d, updated: %d", this->received_cnt, this->updated_cnt);
    
  // here we track how many packets were received
  // since the last update
  // if Navien is connected and we receive packets, the
  // received packet count should be greater than the last time
  // we did an update. If it is not it means we no longer receive packets
  // and therefore should change our status to disconnected
  if (this->received_cnt > this->updated_cnt){
    this->updated_cnt = this->received_cnt;
    this->is_connected = true;
  }else{
    // We've been disconnected. Reset counters
    this->is_connected = false;
    this->received_cnt = 0;
    this->updated_cnt = 0;
  }
      
  if (this->conn_status_sensor != nullptr)
    this->conn_status_sensor->publish_state(this->is_connected);

  update_water_sensors();
  update_gas_sensors();
}

  
void Navien::dump_config(){
  //ESP_LOGCONFIG(TAG, "Calling setup from dump_config");
  //this->setup();
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


#ifdef USE_SWITCH
void NavienOnOffSwitch::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Switch '%s'...", this->name_.c_str());

  bool initial_state = true;//this->get_initial_state_with_restore_mode().value_or(false);

  // write state before setup
  if (initial_state) {
    this->turn_on();
  } else {
    this->turn_off();
  }

}

void NavienOnOffSwitch::write_state(bool state) {
  if (state){
    ESP_LOGD(TAG, "Turning on Navien");
    this->parent->send_turn_on_cmd();
  }else{
    ESP_LOGD(TAG, "Turning off Navien");
    this->parent->send_turn_off_cmd();
  }
  this->publish_state(state);
}

void NavienOnOffSwitch::set_parent(Navien * parent) {
  this->parent = parent;
  parent->set_power_switch(this);
}
  
void NavienOnOffSwitch::dump_config(){
    ESP_LOGCONFIG(TAG, "Empty custom switch");
}
#endif


#ifdef USE_BUTTON
void NavienHotButton::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Button '%s'...", this->name_.c_str());
}

void NavienHotButton::press_action(){
  this->parent->send_hot_button_cmd();
}

void NavienHotButton::set_parent(Navien * parent) {
  this->parent = parent;
  //  parent->set_hot_button(this);
}
  
void NavienHotButton::dump_config(){
    ESP_LOGCONFIG(TAG, "Navien Hot Button");
}
#endif
  
}  // namespace navien
}  // namespace esphome
