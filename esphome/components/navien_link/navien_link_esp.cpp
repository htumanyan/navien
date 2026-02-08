#include "navien_link_esp.h"
#include "esphome/core/log.h"
#include "navien_proto.h"


namespace esphome {
namespace navien {

static const char *TAG = "navien_link_esp.component";

void NavienLinkEsp::setup() {
  // Initialization if needed
}

void NavienLinkEsp::loop() {
  // Call receive to process incoming data
  receive();
}

void NavienLinkEsp::dump_config() {
  ESP_LOGCONFIG(TAG, "Navien Link ESP component");
}

void NavienLinkEsp::on_water(const WATER_DATA &water) {
  ESP_LOGV(TAG, "Received Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, Flow: 0x%02X, Sys Power: 0x%02X, Sys Status: 0x%02X, Recirc Enabled: 0x%02X",
           water.set_temp,
           water.inlet_temp,
           water.outlet_temp,
           water.water_flow,
           water.system_power,
           water.system_status,
           water.recirculation_enabled);

  // Update sensors
  if (target_temp_sensor != nullptr)
    target_temp_sensor->publish_state(NavienLink::t2c(water.set_temp));
  if (inlet_temp_sensor != nullptr)
    inlet_temp_sensor->publish_state(NavienLink::t2c(water.inlet_temp));
  if (outlet_temp_sensor != nullptr)
    outlet_temp_sensor->publish_state(NavienLink::t2c(water.outlet_temp));
  if (water_flow_sensor != nullptr)
    water_flow_sensor->publish_state(NavienLink::flow2lpm(water.water_flow));
  if (water_utilization_sensor != nullptr)
    water_utilization_sensor->publish_state(water.operating_capacity * 0.5f);
  if (recirc_status_sensor != nullptr)
    recirc_status_sensor->publish_state(water.recirculation_enabled & 0x01);
  if (boiler_active_sensor != nullptr)
    boiler_active_sensor->publish_state(water.boiler_active & 0x01);

  // Update climate
  if (climate_ != nullptr) {
    climate_->current_temperature = NavienLink::t2c(water.outlet_temp);
    climate_->target_temperature = NavienLink::t2c(water.set_temp);
    if (water.system_power & 0x01) {
      climate_->mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
    } else {
      climate_->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
    }
    climate_->publish_state();
  }

  // Update switch
  if (power_switch != nullptr) {
    power_switch->publish_state(water.system_power & 0x01);
  }
}

void NavienLinkEsp::on_gas(const GAS_DATA &gas) {
  ESP_LOGV(TAG, "Received Gas data");

  // Update sensors
  if (gas_total_sensor != nullptr)
    gas_total_sensor->publish_state((gas.cumulative_gas_hi << 8 | gas.cumulative_gas_lo) * 0.1f);
  if (gas_current_sensor != nullptr)
    gas_current_sensor->publish_state((gas.current_gas_hi << 8 | gas.current_gas_lo) * 0.1f);
  if (sh_outlet_temperature_sensor != nullptr)
    sh_outlet_temperature_sensor->publish_state(NavienLink::t2c(gas.sh_outlet_temp));
  if (sh_return_temperature_sensor != nullptr)
    sh_return_temperature_sensor->publish_state(NavienLink::t2c(gas.sh_return_temp));
  if (heat_capacity_sensor != nullptr)
    heat_capacity_sensor->publish_state(gas.heat_capacity * 0.5f);
  if (total_dhw_usage_sensor != nullptr)
    total_dhw_usage_sensor->publish_state(gas.cumulative_domestic_usage_cnt_hi << 8 | gas.cumulative_domestic_usage_cnt_lo);
  if (total_operating_time_sensor != nullptr)
    total_operating_time_sensor->publish_state(gas.total_operating_time_hi << 8 | gas.total_operating_time_lo);
  if (total_dhw_usage_hours_sensor != nullptr)
    total_dhw_usage_hours_sensor->publish_state(gas.cumulative_dwh_usage_hours_hi << 8 | gas.cumulative_dwh_usage_hours_lo);
  if (total_sh_usage_hours_sensor != nullptr)
    total_sh_usage_hours_sensor->publish_state(gas.cumulative_sh_usage_hours_hi << 8 | gas.cumulative_sh_usage_hours_lo);

  // Text sensors
  if (device_type_sensor != nullptr) {
    // Implement device type string
    device_type_sensor->publish_state("Unknown");
  }
  if (controller_version_sensor != nullptr) {
    std::string contVers = std::to_string(gas.controller_version_lo);
    if (contVers.length() < 2) contVers.insert(0, 2 - contVers.length(), '0');
    controller_version_sensor->publish_state(contVers.substr(0, 1) + "." + contVers.substr(1, 1));
  }
  if (panel_version_sensor != nullptr) {
    std::string panVers = std::to_string(gas.panel_version_lo);
    if (panVers.length() < 2) panVers.insert(0, 2 - panVers.length(), '0');
    panel_version_sensor->publish_state(panVers.substr(0, 1) + "." + panVers.substr(1, 1));
  }
}

void NavienLinkEsp::on_error(const char *error) {
  ESP_LOGE(TAG, "Error: %s", error);
}

}  // namespace navien
}  // namespace esphome
