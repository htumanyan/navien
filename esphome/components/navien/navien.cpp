#include <cmath>
#include <string>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"
#include "navien_link_esp.h"

namespace esphome {
namespace navien {

  static const char *TAG = "navien.sensor";

  // NavienBase implementation
  void NavienBase::set_link(NavienLinkEsp *link, uint8_t src) {
    this->link_ = link;
    this->src_ = src;
    if (link != nullptr) {
      link->add_visitor(this, src);
    }
  }

  void NavienBase::send_turn_on_cmd() {
    if (this->link_ != nullptr) {
      this->link_->send_turn_on_cmd();
    }
  }

  void NavienBase::send_turn_off_cmd() {
    if (this->link_ != nullptr) {
      this->link_->send_turn_off_cmd();
    }
  }

  void NavienBase::send_hot_button_cmd() {
    if (this->link_ != nullptr) {
      this->link_->send_hot_button_cmd();
    }
  }

  void NavienBase::send_set_temp_cmd(float temp) {
    if (this->link_ != nullptr) {
      this->link_->send_set_temp_cmd(temp);
    }
  }

  // Navien implementation
  void Navien::setup() {
    this->state.power = POWER_OFF;
  }

  void Navien::on_water(const WATER_DATA & water, uint8_t src){
    ESP_LOGD(TAG, "SRC:0x%02X Received Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, Flow: 0x%02X, Sys Power: 0x%02X, Sys Status: 0x%02X, Recirc Enabled: 0x%02X",
             src,
             water.set_temp,
             water.inlet_temp,
             water.outlet_temp,
             water.water_flow,
             water.system_power,
             water.system_status,
             water.recirculation_enabled);

    if ((water.system_power & POWER_STATUS_ON_OFF_MASK) == POWER_STATUS_ON_OFF_MASK){
      state.power = POWER_ON;
    }else{
      state.power = POWER_OFF;
    }

    if ((water.system_status & SYS_STATUS_FLAG_RECIRC) == SYS_STATUS_FLAG_RECIRC){
      state.recirculation = RECIRCULATION_ON;
    }else{
      state.recirculation = RECIRCULATION_OFF;
    }

    
    if ((water.system_status & SYS_STATUS_FLAG_UNITS) == SYS_STATUS_FLAG_UNITS){
      state.units = CELSIUS;
    }else{
      state.units = FARENHEIT;
    }
    state.heating_mode = static_cast<DEVICE_HEATING_MODE>(water.heating_mode);

    state.operating_state = static_cast<OPERATING_STATE>(water.operating_state);
    // Update the counter that will be used in assessment
    // of whether we're connected to navien or not
    this->received_cnt++;
    this->state.water.boiler_active = water.boiler_active & 0x01;
    this->state.water.set_temp = NavienLink::t2c(water.set_temp);
    this->state.water.outlet_temp = NavienLink::t2c(water.outlet_temp);
    this->state.water.inlet_temp = NavienLink::t2c(water.inlet_temp);
    this->state.water.flow_lpm = NavienLink::flow2lpm(water.water_flow);
    this->state.water.utilization = water.operating_capacity * 0.5f;
    if (this->is_rt)
      this->update_water_sensors();
  }

  void Navien::on_gas(const GAS_DATA & gas, uint8_t src){
    ESP_LOGD(TAG, "SRC:0x%02X Received Gas Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X",
       src,
       gas.set_temp,
       gas.inlet_temp,
       gas.outlet_temp
    );

    ESP_LOGD(TAG, "SRC:0x%02X Received Accumulated: 0x%02X 0x%02X, Current Gas: 0x%02X 0x%02X, Capacity Util: 0x%02X",
       src,
       gas.cumulative_gas_hi,
       gas.cumulative_gas_lo,
       gas.current_gas_hi,
       gas.current_gas_lo,
       gas.heat_capacity
    );

    // Update the counter that will be used in assessment
    // of whether we're connected to navien or not
    this->received_cnt++;

    this->state.gas.set_temp    = NavienLink::t2c(gas.set_temp);
    this->state.gas.outlet_temp = NavienLink::t2c(gas.outlet_temp);
    this->state.gas.inlet_temp = NavienLink::t2c(gas.inlet_temp);
    this->state.gas.sh_outlet_temp = NavienLink::t2c(gas.sh_outlet_temp);
    this->state.gas.sh_return_temp = NavienLink::t2c(gas.sh_return_temp);
    this->state.device_type = static_cast<DEVICE_TYPE>(gas.device_type);
    this->state.gas.heat_capacity = gas.heat_capacity * 0.5f;
    this->state.gas.total_dhw_usage = gas.cumulative_domestic_usage_cnt_hi << 8 | gas.cumulative_domestic_usage_cnt_lo;
    this->state.gas.total_operating_time = gas.total_operating_time_hi << 8 | gas.total_operating_time_lo;
    this->state.gas.accumulated_gas_usage = gas.cumulative_gas_hi << 8 | gas.cumulative_gas_lo;
    this->state.gas.current_gas_usage = gas.current_gas_hi << 8 | gas.current_gas_lo;
    this->state.gas.cumulative_dwh_usage_hours = gas.cumulative_dwh_usage_hours_hi << 8 | gas.cumulative_dwh_usage_hours_lo;
    this->state.gas.cumulative_sh_usage_hours = gas.cumulative_sh_usage_hours_hi << 8 | gas.cumulative_sh_usage_hours_lo;

    std::string contVers = std::to_string(gas.controller_version_lo);
    if (contVers.length() < 2){
      contVers.insert(0, 2 - contVers.length(), '0');
    }
    this->state.controller_version = contVers.substr(0, 1) + "." + contVers.substr(1, 1);

    std::string panVers = std::to_string(gas.panel_version_lo);
    if (panVers.length() < 2){
      panVers.insert(0, 2 - panVers.length(), '0');
    }
    this->state.panel_version = panVers.substr(0, 1) + "." + panVers.substr(1, 1);

    this->state.days_since_install = gas.days_since_install_hi << 8 | gas.days_since_install_lo;
    this->state.cumulative_domestic_usage_cnt = gas.cumulative_domestic_usage_cnt_hi << 8 | gas.cumulative_domestic_usage_cnt_lo;

    if (this->is_rt)
      this->update_gas_sensors();
  }

  void Navien::on_error(){
    this->target_temp_sensor->publish_state(0);
    this->outlet_temp_sensor->publish_state(0);
    this->inlet_temp_sensor->publish_state(0);
    this->water_flow_sensor->publish_state(0);
    
    this->sh_outlet_temp_sensor->publish_state(0);
    this->sh_return_temp_sensor->publish_state(0);
    this->heat_capacity_sensor->publish_state(0);
    this->is_connected = false;
  }

  void Navien::update_water_sensors(){
    if (this->water_flow_sensor != nullptr)
      this->water_flow_sensor->publish_state(this->state.water.flow_lpm);

    if (this->water_utilization_sensor != nullptr)
      this->water_utilization_sensor->publish_state(this->state.water.utilization);

    if (this->heating_mode_sensor != nullptr){
      std::string heating_mode_str = heat_mode_to_str(this->state.heating_mode);
      this->heating_mode_sensor->publish_state(heating_mode_str);
    }
    if (this->boiler_active_sensor != nullptr){
      this->boiler_active_sensor->publish_state(this->state.water.boiler_active);
    }
    // Update the climate control with the current target temperature
    if (this->climate != nullptr){
      this->climate->current_temperature = this->state.water.outlet_temp;
      this->climate->target_temperature = this->state.water.set_temp;
      this->climate->publish_state();
    }

    if (this->recirc_status_sensor != nullptr){
      switch(this->state.recirculation){
      case RECIRCULATION_ON:
        this->recirc_status_sensor->publish_state(true);
        break;
      default:
        this->recirc_status_sensor->publish_state(false);
      }
    }

    switch(this->state.power){
    case POWER_ON:
      if (this->power_switch != nullptr)
        this->power_switch->publish_state(true);
      if (this->climate != nullptr){
        this->climate->mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
        this->climate->publish_state();
      }
      break;
    default:
      if (this->power_switch != nullptr)
        this->power_switch->publish_state(false);
      if (this->climate != nullptr){
        this->climate->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
        this->climate->publish_state();
      }
    }

    if (this->power_switch != nullptr){
      switch(this->state.power){
      case POWER_ON:
        this->power_switch->publish_state(true);
        break;
      default:
        this->power_switch->publish_state(false);
      }
    }

    if (this->target_temp_sensor != nullptr)
      this->target_temp_sensor->publish_state(this->state.water.set_temp);
    if (this->outlet_temp_sensor != nullptr)
      this->outlet_temp_sensor->publish_state(this->state.water.outlet_temp);
    if (this->inlet_temp_sensor != nullptr)
      this->inlet_temp_sensor->publish_state(this->state.water.inlet_temp);
    if (this->operating_state_sensor != nullptr){
      std::string operating_state_str = op_state_to_str(this->state.operating_state);
      this->operating_state_sensor->publish_state(operating_state_str);
    }
  }

  void Navien::update_gas_sensors(){
    if (this->target_temp_sensor != nullptr)
      this->target_temp_sensor->publish_state(this->state.gas.set_temp);

    // Update the climate control with the current target temperature
    if (this->climate != nullptr){
      //    this->climate->current_temperature = this->state.gas.outlet_temp * 9.f / 5.f + 32.f;
      //this->climate->target_temperature = this->state.gas.set_temp * 9.f / 5.f + 32.f;
      this->climate->publish_state();
    }
  
  if (this->outlet_temp_sensor != nullptr)
    this->outlet_temp_sensor->publish_state(this->state.gas.outlet_temp);

  if (this->inlet_temp_sensor != nullptr)
      this->inlet_temp_sensor->publish_state(this->state.gas.inlet_temp);

    if (this->gas_total_sensor != nullptr)
      this->gas_total_sensor->publish_state(this->state.gas.accumulated_gas_usage);

    if (this->device_type_sensor != nullptr){
      std::string device_type_str = device_type_to_str(this->state.device_type);
      this->device_type_sensor->publish_state(device_type_str);
    }
    if (this->heat_capacity_sensor != nullptr)
      this->heat_capacity_sensor->publish_state(this->state.gas.heat_capacity);
    if (this->sh_outlet_temp_sensor != nullptr)
      this->sh_outlet_temp_sensor->publish_state(this->state.gas.sh_outlet_temp);
    if (this->sh_return_temp_sensor != nullptr)
      this->sh_return_temp_sensor->publish_state(this->state.gas.sh_return_temp);
    if (this->gas_current_sensor != nullptr)
      this->gas_current_sensor->publish_state(this->state.gas.current_gas_usage);
    if (this->total_dhw_usage_sensor != nullptr)
      this->total_dhw_usage_sensor->publish_state(this->state.gas.total_dhw_usage);
    if (this->total_operating_time_sensor != nullptr)
      this->total_operating_time_sensor->publish_state(this->state.gas.total_operating_time);
    if (this->cumulative_dwh_usage_hours_sensor != nullptr)
      this->cumulative_dwh_usage_hours_sensor->publish_state(this->state.gas.cumulative_dwh_usage_hours);
    if (this->cumulative_sh_usage_hours_sensor != nullptr)
      this->cumulative_sh_usage_hours_sensor->publish_state(this->state.gas.cumulative_sh_usage_hours);
    if (this->cumulative_domestic_usage_cnt_sensor != nullptr)
      this->cumulative_domestic_usage_cnt_sensor->publish_state(this->state.cumulative_domestic_usage_cnt);
    if (this->days_since_install_sensor != nullptr)
      this->days_since_install_sensor->publish_state(this->state.days_since_install);
    if (this->controller_version_sensor != nullptr)
      this->controller_version_sensor->publish_state(this->state.controller_version);
    if (this->panel_version_sensor != nullptr)
      this->panel_version_sensor->publish_state(this->state.panel_version);
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

  std::string Navien::op_state_to_str(OPERATING_STATE state) {
    switch(state){
      case STANDBY:
        return "Standby";
      case STARTUP:
        return "Startup";
      case DEMAND:
        return "Demand";
      case PRE_PURGE_1:
        return "Pre Purge Stage 1";
      case PRE_PURGE_2:
        return "Pre Purge Stage 2";
      case PRE_IGNITION:
        return "Pre-Ignition";
      case IGNITION:
        return "Ignition";
      case FLAME_ON:
        return "Flame On";
      case RAMP_UP:
        return "Ramp Up";
      case ACTIVE_COMBUSTION:
        return "Active Combustion";
      case WATER_ADJUSTMENT_VALVE_OPERATION:
        return "Water Adjustment Valve Operation";
      case FLAME_OFF:
        return "Flame Off";
      case POST_PURGE_1:
        return "Post Purge Stage 1";
      case POST_PURGE_2:
        return "Post Purge Stage 2";
      case DHW_WAIT:
        return "DHW Wait/Set Point Match";
      case SHUTTING_DOWN:
        return "Shutting Down";
      default:
        char buf[64];
        std::snprintf(
            buf,
            sizeof(buf),
            "Unknown (%u)",
            static_cast<unsigned int>(state));

        return buf;
      }
  }

  std::string Navien::heat_mode_to_str(DEVICE_HEATING_MODE mode) {
    switch(mode){
      case HEATING_MODE_IDLE:
        return "Idle";
      case HEATING_MODE_SPACE_HEATING:
        return "Space Heating";
      case HEATING_MODE_DOMESTIC_HOT_WATER_DEMAND:
        return "Domestic Hot Water";
      case HEATING_MODE_DOMESTIC_HOT_WATER_RECIRCULATING:
        return "DHW Recirculating";
      default:
        return "Unknown";
    }
  }

  std::string Navien::device_type_to_str(DEVICE_TYPE type) {
    switch(type){
      case NO_DEVICE:
        return "No Device";
      case NPE:
        return "NPE";
      case NCB:
        return "NCB";
      case NHB:
        return "NHB";
      case CAS_NPE:
        return "CAS NPE";
      case CAS_NHB:
        return "CAS NHB";
      case NFB:
        return "NFB";
      case CAS_NFB:
        return "CAS NFB";
      case NFC:
        return "NFC";
      case NPN:
        return "NPN";
      case CAS_NPN:
        return "CAS NPN";
      case NPE2:
        return "NPE2";
      case CAS_NPE2:
        return "CAS NPE2";
      case NCB_H:
        return "NCB-H";
      case NVW:
        return "NVW";
      case CAS_NVW:
        return "CAS NVW";
      default:
        return "Unknown";
    }
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
