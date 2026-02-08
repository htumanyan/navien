#pragma once

#include "navien_link.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/button/button.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace navien {

/**
 * ESPHome-specific implementation of NavienLink, handling UART and component lifecycle.
 */
class NavienLinkEsp : public ::navien::NavienLink, protected ::navien::NavienUartI, public esphome::uart::UARTDevice, public esphome::Component, public ::navien::NavienLinkVisitorI {
public:
  NavienLinkEsp() : ::navien::NavienLink(this){
    set_listener(this);
  }

  void setup() override;
  void loop() override;
  void dump_config() override;

  // Command methods
  void send_turn_on_cmd() { ::navien::NavienLink::send_turn_on_cmd(); }
  void send_turn_off_cmd() { ::navien::NavienLink::send_turn_off_cmd(); }
  void send_hot_button_cmd() { ::navien::NavienLink::send_hot_button_cmd(); }
  void send_set_temp_cmd(float temp) { ::navien::NavienLink::send_set_temp_cmd(temp); }

  // Sensor setters
  void set_target_temp_sensor(sensor::Sensor *s) { target_temperature_sensor = s; }
  void set_inlet_temp_sensor(sensor::Sensor *s) { inlet_temperature_sensor = s; }
  void set_outlet_temp_sensor(sensor::Sensor *s) { outlet_temperature_sensor = s; }
  void set_water_flow_sensor(sensor::Sensor *s) { water_flow_sensor = s; }
  void set_water_utilization_sensor(sensor::Sensor *s) { water_utilization_sensor = s; }
  void set_gas_total_sensor(sensor::Sensor *s) { gas_total_sensor = s; }
  void set_gas_current_sensor(sensor::Sensor *s) { gas_current_sensor = s; }
  void set_sh_outlet_temperature_sensor(sensor::Sensor *s) { sh_outlet_temperature_sensor = s; }
  void set_sh_return_temperature_sensor(sensor::Sensor *s) { sh_return_temperature_sensor = s; }
  void set_heat_capacity_sensor(sensor::Sensor *s) { heat_capacity_sensor = s; }
  void set_total_dhw_usage_sensor(sensor::Sensor *s) { total_dhw_usage_sensor = s; }
  void set_total_operating_time_sensor(sensor::Sensor *s) { total_operating_time_sensor = s; }
  void set_total_dhw_usage_hours_sensor(sensor::Sensor *s) { total_dhw_usage_hours_sensor = s; }
  void set_total_sh_usage_hours_sensor(sensor::Sensor *s) { total_sh_usage_hours_sensor = s; }
  void set_days_since_install_sensor(sensor::Sensor *s) { days_since_install_sensor = s; }
  void set_conn_status_sensor(binary_sensor::BinarySensor *s) { conn_status_sensor = s; }
  void set_recirc_status_sensor(binary_sensor::BinarySensor *s) { recirc_status_sensor = s; }
  void set_boiler_active_sensor(binary_sensor::BinarySensor *s) { boiler_active_sensor = s; }

  // Text sensor setters
  void set_heating_mode_sensor(text_sensor::TextSensor *s) { heating_mode_sensor = s; }
  void set_device_type_sensor(text_sensor::TextSensor *s) { device_type_sensor = s; }
  void set_operating_state_sensor(text_sensor::TextSensor *s) { operating_state_sensor = s; }
  void set_controller_version_sensor(text_sensor::TextSensor *s) { controller_version_sensor = s; }
  void set_panel_version_sensor(text_sensor::TextSensor *s) { panel_version_sensor = s; }

  // Switch and button setters
  void set_power_switch(switch_::Switch *s) { power_switch = s; }
  void set_hot_button(button::Button *s) { hot_button = s; }

  // Climate setter
  void set_climate(climate::Climate *c) { climate_ = c; }

protected:
  // NavienUartI implementation
  virtual int available() { return esphome::uart::UARTDevice::available(); }
  virtual uint8_t peek_byte(uint8_t *byte) { return esphome::uart::UARTDevice::peek_byte(byte); }
  virtual uint8_t read_byte(uint8_t *byte) { return esphome::uart::UARTDevice::peek_byte(byte); }
  virtual bool read_array(uint8_t *data, uint8_t len) { return esphome::uart::UARTDevice::read_array(data, len); }
  virtual void write_array(const uint8_t *data, uint8_t len) { esphome::uart::UARTDevice::write_array(data, len); }

  // NavienLinkVisitorI implementation
  void on_water(const WATER_DATA &data) override;
  void on_gas(const GAS_DATA &data) override;
  void on_error(const char *error) override;

private:
  // Sensors
  sensor::Sensor *target_temp_sensor = nullptr;
  sensor::Sensor *inlet_temp_sensor = nullptr;
  sensor::Sensor *outlet_temp_sensor = nullptr;
  sensor::Sensor *water_flow_sensor = nullptr;
  sensor::Sensor *water_utilization_sensor = nullptr;
  sensor::Sensor *gas_total_sensor = nullptr;
  sensor::Sensor *gas_current_sensor = nullptr;
  sensor::Sensor *sh_outlet_temperature_sensor = nullptr;
  sensor::Sensor *sh_return_temperature_sensor = nullptr;
  sensor::Sensor *heat_capacity_sensor = nullptr;
  sensor::Sensor *total_dhw_usage_sensor = nullptr;
  sensor::Sensor *total_operating_time_sensor = nullptr;
  sensor::Sensor *total_dhw_usage_hours_sensor = nullptr;
  sensor::Sensor *total_sh_usage_hours_sensor = nullptr;
  sensor::Sensor *days_since_install_sensor = nullptr;
  binary_sensor::BinarySensor *conn_status_sensor = nullptr;
  binary_sensor::BinarySensor *recirc_status_sensor = nullptr;
  binary_sensor::BinarySensor *boiler_active_sensor = nullptr;

  // Text sensors
  text_sensor::TextSensor *heating_mode_sensor = nullptr;
  text_sensor::TextSensor *device_type_sensor = nullptr;
  text_sensor::TextSensor *operating_state_sensor = nullptr;
  text_sensor::TextSensor *controller_version_sensor = nullptr;
  text_sensor::TextSensor *panel_version_sensor = nullptr;

  // Switch and button
  switch_::Switch *power_switch = nullptr;
  button::Button *hot_button = nullptr;

  // Climate
  climate::Climate *climate_ = nullptr;

};

};

}// namespace navien
}// namespace esphome