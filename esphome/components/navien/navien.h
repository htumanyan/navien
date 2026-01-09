#pragma once

#include <cinttypes>
#include <list>

#include "esphome/core/component.h"
// #ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
// #endif

// #ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
// #endif
#include "esphome/components/text_sensor/text_sensor.h"

// #ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
// #endif
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"

#include "navien_link.h"

#ifndef USE_SWITCH
namespace switch_
{
  class Switch;
}
#endif

namespace esphome
{
  namespace navien
  {

    typedef enum _DEVICE_POWER_STATE
    {
      POWER_OFF,
      POWER_ON
    } DEVICE_POWER_STATE;

    typedef enum _DEVICE_RECIRC_STATE
    {
      RECIRCULATION_OFF,
      RECIRCULATION_ON
    } DEVICE_RECIRC_STATE;

    typedef enum _DEVICE_UNITS
    {
      CELSIUS,
      FARENHEIT
    } DEVICE_UNITS;

    typedef enum _DEVICE_HEATING_MODE
    {
      HEATING_MODE_IDLE = 0x00,
      HEATING_MODE_DOMESTIC_HOT_WATER_RECIRCULATING = 0x08,
      HEATING_MODE_SPACE_HEATING = 0x10,
      HEATING_MODE_DOMESTIC_HOT_WATER_DEMAND = 0x20
    } DEVICE_HEATING_MODE;


//https://github.com/rudybrian/PyNavienSmartController
    typedef enum _DEVICE_TYPE
    {
      NO_DEVICE,
      NPE,
      NCB,
      NHB,
      CAS_NPE,
      CAS_NHB,
      NFB,
      CAS_NFB,
      NFC,
      NPN,
      CAS_NPN,
      NPE2,
      CAS_NPE2,
      NCB_H,
      NVW,
      CAS_NVW
    } DEVICE_TYPE;


//https://github.com/ificator/navien_ha
    typedef enum _OPERATING_STATE
    {
      STANDBY = 0x14,
      DEMAND = 0x15,
      STARTUP = 0x20, 
      PRE_PURGE_1 = 0x28,
      PRE_PURGE_2 = 0x29,
      PRE_IGNITION = 0x2A,
      IGNITION = 0x2B,
      FLAME_ON = 0x2C,
      RAMP_UP = 0x2D,
      ACTIVE_COMBUSTION = 0x33,
      WATER_ADJUSTMENT_VALVE_OPERATION = 0x34,
      FLAME_OFF = 0x3C,
      POST_PURGE_1 = 0x46,
      POST_PURGE_2 = 0x47,
      DHW_WAIT = 0x49 //DHW Wait / Set Point Match depending on model
    } OPERATING_STATE;

    typedef struct
    {
      struct
      {
        uint8_t set_temp;
        uint8_t outlet_temp;
        uint8_t inlet_temp;
        float flow_lpm;
        uint8_t utilization;
        bool boiler_active;

      } water;
      struct
      {
        uint8_t set_temp;
        uint8_t outlet_temp;
        uint8_t inlet_temp;
        uint16_t accumulated_gas_usage;
        uint16_t current_gas_usage;
        uint8_t sh_outlet_temp; // combi (and space heat?) models
        uint8_t sh_return_temp; // combi (and space heat?) models
        uint8_t heat_capacity;
        uint16_t total_dhw_usage;
        uint16_t total_operating_time;
        uint16_t cumulative_dwh_usage_hours;
        uint16_t cumulative_sh_usage_hours;
        uint16_t cumulative_domestic_usage_cnt;
        uint16_t days_since_install;
      } gas;
      struct
      {
        uint8_t unknown_06;
        uint8_t unknown_07;
        uint8_t unknown_10;
        uint8_t unknown_14;
        uint8_t unknown_15;
        uint8_t unknown_17;
        uint8_t unknown_20;
        uint8_t unknown_21;
        uint8_t unknown_22;
        uint8_t unknown_23;
        uint8_t unknown_24;
        uint8_t unknown_25;
        uint8_t unknown_26;
        uint8_t unknown_28;
        uint8_t unknown_29;
        uint8_t unknown_30;
        uint8_t unknown_31;
        uint8_t unknown_32;
        uint8_t unknown_34;
        uint8_t unknown_35;
        uint8_t unknown_36;
        uint8_t unknown_37;
        uint8_t unknown_38;
        uint8_t unknown_39;
      } water_unknowns;

      struct
      {
        uint8_t unknown_00;
        uint8_t unknown_01;
        uint8_t unknown_03;
        uint8_t unknown_18;
        uint8_t unknown_20; /// 0x05 always so far
        uint8_t unknown_26;
        uint8_t unknown_27;
        uint8_t unknown_32;
        uint8_t unknown_33;
        uint8_t unknown_34;
        uint8_t unknown_35;
        uint8_t unknown_42;
        uint8_t unknown_43;
        uint8_t unknown_44;
        uint8_t unknown_45;
        uint8_t unknown_46;
        uint8_t unknown_47;
      } gas_unknowns;

      u_int16_t cumulative_domestic_usage_cnt;
      uint16_t days_since_install;
      std::string controller_version;
      std::string panel_version;
      OPERATING_STATE operating_state;
      DEVICE_HEATING_MODE heating_mode;
      DEVICE_TYPE device_type;
      DEVICE_POWER_STATE power;
      DEVICE_RECIRC_STATE recirculation;
      DEVICE_UNITS units;
    } NAVIEN_STATE;

    class NavienBase : public uart::UARTDevice, protected NavienUartI, public NavienLinkVisitorI
    {
    public:
      NavienBase() : navien_link(*this, *this), is_rt(false) {}

    public:
      void set_target_temp_sensor(sensor::Sensor *sensor) { target_temp_sensor = sensor; }
      void set_inlet_temp_sensor(sensor::Sensor *sensor) { inlet_temp_sensor = sensor; }
      void set_outlet_temp_sensor(sensor::Sensor *sensor) { outlet_temp_sensor = sensor; }
      void set_gas_target_temp_sensor(sensor::Sensor *sensor) { gas_target_temp_sensor = sensor; }
      void set_gas_inlet_temp_sensor(sensor::Sensor *sensor) { gas_inlet_temp_sensor = sensor; }
      void set_gas_outlet_temp_sensor(sensor::Sensor *sensor) { gas_outlet_temp_sensor = sensor; }
      void set_water_flow_sensor(sensor::Sensor *sensor) { water_flow_sensor = sensor; }
      void set_water_utilization_sensor(sensor::Sensor *sensor) { water_utilization_sensor = sensor; }
      void set_gas_total_sensor(sensor::Sensor *sensor) { gas_total_sensor = sensor; }
      void set_gas_current_sensor(sensor::Sensor *sensor) { gas_current_sensor = sensor; }
      void set_device_type_sensor(text_sensor::TextSensor *sensor) { device_type_sensor = sensor; }
      void set_operating_state_sensor(text_sensor::TextSensor *sensor) { operating_state_sensor = sensor; }
      void set_real_time(bool rt) { this->is_rt = rt; }
      void set_heating_mode_sensor(text_sensor::TextSensor *sensor) { heating_mode_sensor = sensor; }
      void set_conn_status_sensor(binary_sensor::BinarySensor *sensor) { conn_status_sensor = sensor; }
      void set_recirc_status_sensor(binary_sensor::BinarySensor *sensor) { recirc_status_sensor = sensor; }
      void set_sh_outlet_temp_sensor(sensor::Sensor *sensor) { sh_outlet_temp_sensor = sensor; }
      void set_sh_return_temp_sensor(sensor::Sensor *sensor) { sh_return_temp_sensor = sensor; }
      void set_heat_capacity_sensor(sensor::Sensor *sensor) { heat_capacity_sensor = sensor; } 
      void set_boiler_active_sensor(binary_sensor::BinarySensor *sensor) { boiler_active_sensor = sensor; } 
      void set_days_since_install_sensor(sensor::Sensor *sensor) { days_since_install_sensor = sensor; }

      void set_unk_sensor_w06(sensor::Sensor *sensor) { unk_sensor_w06 = sensor; }
      void set_unk_sensor_w07(sensor::Sensor *sensor) { unk_sensor_w07 = sensor; }
      void set_unk_sensor_w14(sensor::Sensor *sensor) { unk_sensor_w14 = sensor; }
      void set_unk_sensor_w15(sensor::Sensor *sensor) { unk_sensor_w15 = sensor; }
      void set_unk_sensor_w17(sensor::Sensor *sensor) { unk_sensor_w17 = sensor; }
      void set_unk_sensor_w20(sensor::Sensor *sensor) { unk_sensor_w20 = sensor; }
      void set_unk_sensor_w21(sensor::Sensor *sensor) { unk_sensor_w21 = sensor; }
      void set_unk_sensor_w22(sensor::Sensor *sensor) { unk_sensor_w22 = sensor; }
      void set_unk_sensor_w23(sensor::Sensor *sensor) { unk_sensor_w23 = sensor; }
      void set_unk_sensor_w24(sensor::Sensor *sensor) { unk_sensor_w24 = sensor; }
      void set_unk_sensor_w25(sensor::Sensor *sensor) { unk_sensor_w25 = sensor; }
      void set_unk_sensor_w26(sensor::Sensor *sensor) { unk_sensor_w26 = sensor; }
      void set_unk_sensor_w28(sensor::Sensor *sensor) { unk_sensor_w28 = sensor; }
      void set_unk_sensor_w29(sensor::Sensor *sensor) { unk_sensor_w29 = sensor; }
      void set_unk_sensor_w30(sensor::Sensor *sensor) { unk_sensor_w30 = sensor; }
      void set_unk_sensor_w31(sensor::Sensor *sensor) { unk_sensor_w31 = sensor; }
      void set_unk_sensor_w32(sensor::Sensor *sensor) { unk_sensor_w32 = sensor; }
      void set_unk_sensor_w34(sensor::Sensor *sensor) { unk_sensor_w34 = sensor; }
      void set_unk_sensor_w35(sensor::Sensor *sensor) { unk_sensor_w35 = sensor; }
      void set_unk_sensor_w36(sensor::Sensor *sensor) { unk_sensor_w36 = sensor; }
      void set_unk_sensor_w37(sensor::Sensor *sensor) { unk_sensor_w37 = sensor; }
      void set_unk_sensor_w38(sensor::Sensor *sensor) { unk_sensor_w38 = sensor; }
      void set_unk_sensor_w39(sensor::Sensor *sensor) { unk_sensor_w39 = sensor; }
      void set_unk_sensor_g00(sensor::Sensor *sensor) { unk_sensor_g00 = sensor; }
      void set_unk_sensor_g01(sensor::Sensor *sensor) { unk_sensor_g01 = sensor; }
      void set_unk_sensor_g03(sensor::Sensor *sensor) { unk_sensor_g03 = sensor; }
      void set_unk_sensor_g18(sensor::Sensor *sensor) { unk_sensor_g18 = sensor; }
      void set_unk_sensor_g20(sensor::Sensor *sensor) { unk_sensor_g20 = sensor; }
      void set_unk_sensor_g26(sensor::Sensor *sensor) { unk_sensor_g26 = sensor; }
      void set_unk_sensor_g27(sensor::Sensor *sensor) { unk_sensor_g27 = sensor; }
      void set_unk_sensor_g32(sensor::Sensor *sensor) { unk_sensor_g32 = sensor; }
      void set_unk_sensor_g33(sensor::Sensor *sensor) { unk_sensor_g33 = sensor; }
      void set_unk_sensor_g34(sensor::Sensor *sensor) { unk_sensor_g34 = sensor; }
      void set_unk_sensor_g35(sensor::Sensor *sensor) { unk_sensor_g35 = sensor; }
      void set_unk_sensor_g42(sensor::Sensor *sensor) { unk_sensor_g42 = sensor; }
      void set_unk_sensor_g43(sensor::Sensor *sensor) { unk_sensor_g43 = sensor; }
      void set_unk_sensor_g44(sensor::Sensor *sensor) { unk_sensor_g44 = sensor; }
      void set_unk_sensor_g45(sensor::Sensor *sensor) { unk_sensor_g45 = sensor; }
      void set_unk_sensor_g46(sensor::Sensor *sensor) { unk_sensor_g46 = sensor; }
      void set_unk_sensor_g47(sensor::Sensor *sensor) { unk_sensor_g47 = sensor; }
      void set_total_dhw_usage_sensor(sensor::Sensor *sensor) { total_dhw_usage_sensor = sensor; }
      void set_total_operating_time_sensor(sensor::Sensor *sensor) { total_operating_time_sensor = sensor; }
      void set_controller_version_sensor(text_sensor::TextSensor *sensor) { controller_version_sensor = sensor; }
      void set_panel_version_sensor(text_sensor::TextSensor *sensor) { panel_version_sensor = sensor; }
      void set_cumulative_dwh_usage_hours_sensor(sensor::Sensor *sensor) { cumulative_dwh_usage_hours_sensor = sensor; }
      void set_cumulative_sh_usage_hours_sensor(sensor::Sensor *sensor) { cumulative_sh_usage_hours_sensor = sensor; }
      void set_cumulative_domestic_usage_cnt_sensor(sensor::Sensor *sensor) { cumulative_domestic_usage_cnt_sensor = sensor; }
      /**
       * Sets the power switch component that will be notified of power
       * state changes, i.e. if the unit goes off, Navien will updte the switch to reflect that
       */
      void set_power_switch(switch_::Switch *ps) { power_switch = ps; }

      void set_climate(climate::Climate *c) { climate = c; }

      /**
       * Sets the climate component that will be receiving temperature updates
       */
      // void set_climate(switch_::Switch * ps){power_switch = ps;}

      void send_turn_on_cmd() { this->navien_link.send_turn_on_cmd(); }
      void send_turn_off_cmd() { this->navien_link.send_turn_off_cmd(); }
      void send_hot_button_cmd() { this->navien_link.send_hot_button_cmd(); }
      void send_set_temp_cmd(float temp) { this->navien_link.send_set_temp_cmd(temp); }

    protected:
      /**
       * NavienUartI interface implementation for ESPHome
       */
      virtual int available() { return uart::UARTDevice::available(); }
      virtual uint8_t peek_byte(uint8_t *byte) { return uart::UARTDevice::peek_byte(byte); }
      virtual uint8_t read_byte(uint8_t *byte) { return uart::UARTDevice::read_byte(byte); }
      virtual bool read_array(uint8_t *data, uint8_t len) { return uart::UARTDevice::read_array(data, len); }
      virtual void write_array(const uint8_t *data, uint8_t len) { uart::UARTDevice::write_array(data, len); }

    protected:
      NavienLink navien_link;

      /**
       * Sensor definitions
       */

      sensor::Sensor *target_temp_sensor = nullptr;
      sensor::Sensor *outlet_temp_sensor = nullptr;
      sensor::Sensor *inlet_temp_sensor = nullptr;
      sensor::Sensor *gas_target_temp_sensor = nullptr;
      sensor::Sensor *gas_outlet_temp_sensor = nullptr;
      sensor::Sensor *gas_inlet_temp_sensor = nullptr;
      sensor::Sensor *water_flow_sensor = nullptr;
      sensor::Sensor *water_utilization_sensor = nullptr;
      sensor::Sensor *gas_total_sensor = nullptr;
      sensor::Sensor *gas_current_sensor = nullptr;
      text_sensor::TextSensor *heating_mode_sensor = nullptr;
      text_sensor::TextSensor *device_type_sensor = nullptr;
      text_sensor::TextSensor *operating_state_sensor = nullptr;
      sensor::Sensor *sh_outlet_temp_sensor = nullptr;
      sensor::Sensor *sh_return_temp_sensor = nullptr;
      sensor::Sensor *heat_capacity_sensor = nullptr;
      sensor::Sensor *total_dhw_usage_sensor = nullptr;
      sensor::Sensor *total_operating_time_sensor = nullptr;
      sensor::Sensor *cumulative_dwh_usage_hours_sensor = nullptr;
      sensor::Sensor *cumulative_sh_usage_hours_sensor = nullptr;
      sensor::Sensor *cumulative_domestic_usage_cnt_sensor = nullptr;
      sensor::Sensor *days_since_install_sensor = nullptr;
      text_sensor::TextSensor *controller_version_sensor = nullptr;
      text_sensor::TextSensor *panel_version_sensor = nullptr;
      // dev unk sensors
      sensor::Sensor *unk_sensor_w06 = nullptr;
      sensor::Sensor *unk_sensor_w07 = nullptr;
      sensor::Sensor *unk_sensor_w14 = nullptr;
      sensor::Sensor *unk_sensor_w15 = nullptr;
      sensor::Sensor *unk_sensor_w17 = nullptr;
      sensor::Sensor *unk_sensor_w20 = nullptr;
      sensor::Sensor *unk_sensor_w21 = nullptr;
      sensor::Sensor *unk_sensor_w22 = nullptr;
      sensor::Sensor *unk_sensor_w23 = nullptr;
      sensor::Sensor *unk_sensor_w24 = nullptr;
      sensor::Sensor *unk_sensor_w25 = nullptr;
      sensor::Sensor *unk_sensor_w26 = nullptr;
      sensor::Sensor *unk_sensor_w28 = nullptr;
      sensor::Sensor *unk_sensor_w29 = nullptr;
      sensor::Sensor *unk_sensor_w30 = nullptr;
      sensor::Sensor *unk_sensor_w31 = nullptr;
      sensor::Sensor *unk_sensor_w32 = nullptr;
      sensor::Sensor *unk_sensor_w34 = nullptr;
      sensor::Sensor *unk_sensor_w35 = nullptr;
      sensor::Sensor *unk_sensor_w36 = nullptr;
      sensor::Sensor *unk_sensor_w37 = nullptr;
      sensor::Sensor *unk_sensor_w38 = nullptr;
      sensor::Sensor *unk_sensor_w39 = nullptr;

      sensor::Sensor *unk_sensor_g00 = nullptr;
      sensor::Sensor *unk_sensor_g01 = nullptr;
      sensor::Sensor *unk_sensor_g03 = nullptr;
      sensor::Sensor *unk_sensor_g18 = nullptr;
      sensor::Sensor *unk_sensor_g20 = nullptr;
      sensor::Sensor *unk_sensor_g26 = nullptr;
      sensor::Sensor *unk_sensor_g27 = nullptr;
      sensor::Sensor *unk_sensor_g32 = nullptr;
      sensor::Sensor *unk_sensor_g33 = nullptr;
      sensor::Sensor *unk_sensor_g34 = nullptr;
      sensor::Sensor *unk_sensor_g35 = nullptr;
      sensor::Sensor *unk_sensor_g42 = nullptr;
      sensor::Sensor *unk_sensor_g43 = nullptr;
      sensor::Sensor *unk_sensor_g44 = nullptr;
      sensor::Sensor *unk_sensor_g45 = nullptr;
      sensor::Sensor *unk_sensor_g46 = nullptr;
      sensor::Sensor *unk_sensor_g47 = nullptr;

      binary_sensor::BinarySensor *boiler_active_sensor = nullptr;
      binary_sensor::BinarySensor *conn_status_sensor = nullptr;
      binary_sensor::BinarySensor *recirc_status_sensor = nullptr;

      switch_::Switch *power_switch = nullptr;
      climate::Climate *climate = nullptr;

      bool is_rt;
    };

    class Navien : public PollingComponent, public NavienBase
    {
    public:
      Navien()
      {
        updated_cnt = 0;
        received_cnt = 0;
        is_connected = false;
      }

      virtual float get_setup_priority() const { return setup_priority::HARDWARE; }
      virtual void setup() override;
      void update() override;
      void dump_config() override;
      // Component::loop implementation that simply calls receive to engage in
      // receipt of stats over rs485 using the UART class embedded here.
      void loop() override
      {
        this->navien_link.receive();
      }

    protected:
      // Debug helper to print hex buffers
      static void print_buffer(const uint8_t *data, size_t length);

    protected:
      /**
       * Helper functions to be used for updating sensor states either in real time or
       * on timer if used in a polling component
       */
      virtual void update_water_sensors();
      virtual void update_gas_sensors();

    protected:
      // Data, extracted from gas and water packers and stored
      // Once the "update" is called this data gets reported to readers.
      NAVIEN_STATE state = {};

    protected:
      /**
       * NavienLinkVisitorI interface implementation
       */
      virtual void on_water(const WATER_DATA &water);
      virtual void on_gas(const GAS_DATA &gas);
      virtual void on_error();

    protected:
      /**
       * These two variabls keep track of how many updates
       * have we received from Navien and how many we sent updates
       * to HA. The former should be > the latter if we keep receiving
       * statuses from navien since the last update to HA, which means we
       * are connected.
       */
      // How many packets were received
      uint32_t received_cnt;

      // How many packets were updated
      uint32_t updated_cnt;

      // true if connected to Navien.
      // otherwie - false.
      bool is_connected;
    };

#ifdef USE_SWITCH
    class NavienOnOffSwitch : public switch_::Switch, public Component
    {
    protected:
      Navien *parent = nullptr;

    public:
      void setup() override;

      void set_parent(Navien *parent);
      void write_state(bool state) override;
      void dump_config() override;
    };
#endif

#ifdef USE_BUTTON
    class NavienHotButton : public button::Button, public Component
    {
    protected:
      Navien *parent = nullptr;

    public:
      void setup() override;

      void set_parent(Navien *parent);

      void press_action() override;
      void dump_config() override;
    };
#endif

  } // namespace navien
} // namespace esphome
