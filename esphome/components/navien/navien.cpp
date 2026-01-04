#include <cmath>
#include <string>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"

namespace esphome {
namespace navien {

  static const char *TAG = "navien.sensor";

  void Navien::setup() {
    this->state.power = POWER_OFF;
  }

  void Navien::on_water(const WATER_DATA & water){
    ESP_LOGV(TAG, "Received Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, Flow: 0x%02X, Sys Power: 0x%02X, Sys Status: 0x%02X, Recirc Enabled: 0x%02X",
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

    if (water.system_status & SYS_STATUS_FLAG_RECIRC){
      state.recirculation = RECIRCULATION_ON;
    }else{
      state.recirculation = RECIRCULATION_OFF;
    }

    
    if (water.system_status & SYS_STATUS_FLAG_UNITS){
      state.units = CELSIUS;
    }else{
      state.units = FARENHEIT;
    }
    state.heating_mode = static_cast<DEVICE_HEATING_MODE>(water.heating_mode);

    state.operating_state = static_cast<OPERATING_STATE>(water.operating_state);
    // Update the counter that will be used in assessment
    // of whether we're connected to navien or not
    this->received_cnt++;
    this->state.water.boiler_active = static_cast<bool>(water.boiler_active);
    this->state.water.set_temp = NavienLink::t2c(water.set_temp);
    this->state.water.outlet_temp = NavienLink::t2c(water.outlet_temp);
    this->state.water.inlet_temp = NavienLink::t2c(water.inlet_temp);
    this->state.water.flow_lpm = NavienLink::flow2lpm(water.water_flow);
    this->state.water.utilization = water.operating_capacity * 0.5f;
    if (this->is_rt)
      this->update_water_sensors();
  }

  void Navien::on_gas(const GAS_DATA & gas){
    ESP_LOGV(TAG, "Received Gas Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X",
       gas.set_temp,
       gas.inlet_temp,
       gas.outlet_temp
    );

    ESP_LOGV(TAG, "Received Accumulated: 0x%02X 0x%02X, Current Gas: 0x%02X 0x%02X, Capacity Util: 0x%02X",
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
      std::string heating_mode_str;
      switch(this->state.heating_mode){
      case HEATING_MODE_IDLE:
        heating_mode_str = "Idle";
        break;
      case HEATING_MODE_SPACE_HEATING:
        heating_mode_str = "Space Heating";
        break;
      case HEATING_MODE_DOMESTIC_HOT_WATER_DEMAND:
        heating_mode_str = "Domestic Hot Water";
        break;
      case HEATING_MODE_DOMESTIC_HOT_WATER_RECIRCULATING:
        heating_mode_str = "DHW Recirculating";
        break;
      default:
        heating_mode_str = "Unknown";
      }
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
      std::string operating_state_str;
      switch(this->state.operating_state){
      case STANDBY:
        operating_state_str = "Standby";
        break;
      case STARTUP:
        operating_state_str = "Startup";
        break;
      case DEMAND:
        operating_state_str = "Demand";
        break;
      case PRE_PURGE_1:
        operating_state_str = "Pre Purge Stage 1";
        break;
      case PRE_PURGE_2:
        operating_state_str = "Pre Purge Stage 2";
        break;
      case PRE_IGNITION:
        operating_state_str = "Pre-Ignition";
        break;
      case IGNITION:
        operating_state_str = "Ignition";
        break;
      case FLAME_ON:
        operating_state_str = "Flame On";
        break;
      case RAMP_UP:
        operating_state_str = "Ramp Up";
        break;
      case ACTIVE_COMBUSTION:
        operating_state_str = "Active Combustion";
        break;
      case WATER_ADJUSTMENT_VALVE_OPERATION:
        operating_state_str = "Water Adjustment Valve Operation";
        break;
      case FLAME_OFF:
        operating_state_str = "Flame Off";
        break;
      case POST_PURGE_1:
        operating_state_str = "Post Purge Stage 1";
        break;
      case POST_PURGE_2:
        operating_state_str = "Post Purge Stage 2";
        break;
      case DHW_WAIT:
        operating_state_str = "DHW Wait/Set Point Match";
        break;
      default:
        char buf[64];
        std::snprintf(
            buf,
            sizeof(buf),
            "Unknown (%u)",
            static_cast<unsigned int>(this->state.operating_state));

        operating_state_str = buf;
        break;
      }
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
      std::string device_type_str;
      switch (this->state.device_type)
      {
      case NO_DEVICE:
        device_type_str = "No Device";
        break;
      case NPE:
        device_type_str = "NPE";
        break;
      case NCB:
        device_type_str = "NCB";
        break;
      case NHB:
        device_type_str = "NHB";
        break;
      case CAS_NPE:
        device_type_str = "CAS NPE";
        break;
      case CAS_NHB:
        device_type_str = "CAS NHB";
        break;
      case NFB:
        device_type_str = "NFB";
        break;
      case CAS_NFB:
        device_type_str = "CAS NFB";
        break;
      case NFC:
        device_type_str = "NFC";
        break;
      case NPN:
        device_type_str = "NPN";
        break;
      case CAS_NPN:
        device_type_str = "CAS NPN";
        break;
      case NPE2:
        device_type_str = "NPE2";
        break;
      case CAS_NPE2:
        device_type_str = "CAS NPE2";
        break;
      case NCB_H:
        device_type_str = "NCB-H";
        break;
      case NVW:
        device_type_str = "NVW";
        break;
      case CAS_NVW:
        device_type_str = "CAS NVW";
        break;
      default:
        device_type_str = "Unknown";
        break;
      }
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
