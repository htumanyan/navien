#pragma once

#include <cinttypes>
#include <list>

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_CLIMATE
#include "esphome/components/climate/climate.h"
#endif

#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

#ifdef USE_WATER_HEATER
#include "water_heater/navien_water_heater.h"
#endif

#include "navien_link.h"
#include "navien_proto.h"

namespace esphome {
namespace navien {


  typedef enum _DEVICE_POWER_STATE{
    POWER_OFF,
    POWER_ON
  } DEVICE_POWER_STATE;

  
  typedef enum _DEVICE_RECIRC_MODE{
    RECIRC_UNKNOWN,
    RECIRC_OFF,
    RECIRC_EXT_HOTBUTTON,
    RECIRC_EXT_SCHEDULED,
    RECIRC_INT_SCHEDULED,
  } DEVICE_RECIRC_MODE;

  typedef enum _DEVICE_UNITS{
    CELSIUS,
    FARENHEIT
  } DEVICE_UNITS;

  typedef enum _DEVICE_HEATING_MODE{
    HEATING_MODE_IDLE = 0x00,
    HEATING_MODE_DOMESTIC_HOT_WATER_RECIRCULATING = 0x08,
    HEATING_MODE_SPACE_HEATING = 0x10,
    HEATING_MODE_DOMESTIC_HOT_WATER_DEMAND = 0x20
  } DEVICE_HEATING_MODE;


  //based on https://github.com/rudybrian/PyNavienSmartController
  typedef enum _DEVICE_TYPE{
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


  //based on https://github.com/ificator/navien_ha
  typedef enum _OPERATING_STATE{
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
    SHUTTING_DOWN = 0x48,
    DHW_WAIT = 0x49 //DHW Wait / Set Point Match depending on model
  } OPERATING_STATE;

  typedef struct{
    struct{
      float dhw_set_temp;
      float outlet_temp;
      float inlet_temp;
      float flow_lpm;
      uint8_t utilization;
      bool boiler_active;
      bool scheduled_recirc_allowed;
      bool recirc_running;
    } water;
    struct{
      float  dhw_set_temp;
      float  outlet_temp;
      float  inlet_temp;
      uint16_t accumulated_gas_usage;
      uint16_t current_gas_usage;
      float  sh_set_temp;
      float  sh_outlet_temp; // combi (and space heat?) models
      float  sh_return_temp; // combi (and space heat?) models
      float  outdoor_temp;
      uint8_t heat_capacity;
      uint16_t total_dhw_usage;
      uint16_t total_operating_time;
      uint16_t cumulative_dwh_usage_hours;
      uint16_t cumulative_sh_usage_hours;
      uint16_t cumulative_domestic_usage_cnt;
      uint16_t days_since_install;
    } gas;

    uint16_t cumulative_domestic_usage_cnt;
    uint16_t days_since_install;
    std::string controller_version;
    std::string panel_version;
    OPERATING_STATE operating_state;
    DEVICE_HEATING_MODE heating_mode;
    DEVICE_TYPE device_type;
    DEVICE_POWER_STATE  power;
    DEVICE_RECIRC_MODE  recirculation;
    DEVICE_UNITS        units;
    bool hotbutton_mode_enabled;  // From GAS packet - true if HotButton mode is configured
  } NAVIEN_STATE;


  // Forward declaration

  class NavienBase : public NavienLinkVisitorI {
  public:
    NavienBase();

    virtual void setup();

    void set_uart(esphome::uart::UARTComponent* uart) { uart_ = uart; }
    void set_src(uint8_t src) { src_ = src; }

    void send_turn_on_cmd();
    void send_turn_off_cmd();
    void send_hot_button_cmd();
    void send_dhw_set_temp_cmd(float temp);
    void send_scheduled_recirculation_on_cmd();
    void send_scheduled_recirculation_off_cmd();

  public:
    void set_dhw_set_temp_sensor(sensor::Sensor *sensor) { dhw_set_temp_sensor = sensor; }
    void set_inlet_temp_sensor(sensor::Sensor *sensor) { inlet_temp_sensor = sensor; }
    void set_outlet_temp_sensor(sensor::Sensor *sensor) { outlet_temp_sensor = sensor; }
    void set_gas_dhw_set_temp_sensor(sensor::Sensor *sensor) { gas_dhw_set_temp_sensor = sensor; }
    void set_gas_inlet_temp_sensor(sensor::Sensor *sensor) { gas_inlet_temp_sensor = sensor; }
    void set_gas_outlet_temp_sensor(sensor::Sensor *sensor) { gas_outlet_temp_sensor = sensor; }
    void set_water_flow_sensor(sensor::Sensor *sensor) { water_flow_sensor = sensor; }
    void set_water_utilization_sensor(sensor::Sensor *sensor) { water_utilization_sensor = sensor; }
    void set_recirc_running_sensor(binary_sensor::BinarySensor *sensor) { recirc_running_sensor = sensor; }
    void set_gas_total_sensor(sensor::Sensor *sensor) { gas_total_sensor = sensor; }
    void set_gas_current_sensor(sensor::Sensor *sensor) { gas_current_sensor = sensor; }
    void set_device_type_sensor(text_sensor::TextSensor *sensor) { device_type_sensor = sensor; }
    void set_operating_state_sensor(text_sensor::TextSensor *sensor) { operating_state_sensor = sensor; }
    void set_real_time(bool rt){this->is_rt = rt;}

    void set_heating_mode_sensor(text_sensor::TextSensor *sensor) { heating_mode_sensor = sensor; }
    void set_conn_status_sensor(binary_sensor::BinarySensor *sensor) { conn_status_sensor = sensor; }
    void set_recirc_mode_sensor(text_sensor::TextSensor *sensor) { recirc_mode_sensor = sensor; }  
    void set_sh_set_temp_sensor(sensor::Sensor *sensor) { sh_set_temp_sensor = sensor; }
    void set_sh_outlet_temp_sensor(sensor::Sensor *sensor) { sh_outlet_temp_sensor = sensor; }
    void set_sh_return_temp_sensor(sensor::Sensor *sensor) { sh_return_temp_sensor = sensor; }
    void set_outdoor_temp_sensor(sensor::Sensor *sensor) { outdoor_temp_sensor = sensor; }
    void set_heat_capacity_sensor(sensor::Sensor *sensor) { heat_capacity_sensor = sensor; } 
    void set_boiler_active_sensor(binary_sensor::BinarySensor *sensor) { boiler_active_sensor = sensor; } 
    void set_days_since_install_sensor(sensor::Sensor *sensor) { days_since_install_sensor = sensor; }
    void set_total_dhw_usage_sensor(sensor::Sensor *sensor) { total_dhw_usage_sensor = sensor; }
    void set_total_operating_time_sensor(sensor::Sensor *sensor) { total_operating_time_sensor = sensor; }
    void set_controller_version_sensor(text_sensor::TextSensor *sensor) { controller_version_sensor = sensor; }
    void set_panel_version_sensor(text_sensor::TextSensor *sensor) { panel_version_sensor = sensor; }
    void set_cumulative_dwh_usage_hours_sensor(sensor::Sensor *sensor) { cumulative_dwh_usage_hours_sensor = sensor; }
    void set_cumulative_sh_usage_hours_sensor(sensor::Sensor *sensor) { cumulative_sh_usage_hours_sensor = sensor; }
    void set_cumulative_domestic_usage_cnt_sensor(sensor::Sensor *sensor) { cumulative_domestic_usage_cnt_sensor = sensor; }
    void set_other_navilink_installed_sensor(binary_sensor::BinarySensor *sensor) { other_navilink_installed_sensor = sensor; }

#ifdef USE_SWITCH
    /**
     * Sets the power switch component that will be notified of power
     * state changes, i.e. if the unit goes off, Navien will update the switch to reflect that
     */
    void set_power_switch(switch_::Switch * ps){power_switch = ps;}

    /**
     * Sets the allow recirculation switch component that will be notified of
     * when scheduled recirculation is active or inactive
     */
    void set_allow_recirc_switch(switch_::Switch * rs){allow_recirc_switch = rs;}
#endif

#ifdef USE_CLIMATE
    void set_climate(climate::Climate * c){climate = c;}
#endif

#ifdef USE_WATER_HEATER
    void set_water_heater(NavienWaterHeater * w){water_heater = w;}
#endif

  protected:
    /**
     * Sensor definitions
     */

    sensor::Sensor *dhw_set_temp_sensor = nullptr;
    sensor::Sensor *outlet_temp_sensor = nullptr;
    sensor::Sensor *inlet_temp_sensor = nullptr;
    sensor::Sensor *gas_dhw_set_temp_sensor = nullptr;
    sensor::Sensor *gas_outlet_temp_sensor = nullptr;
    sensor::Sensor *gas_inlet_temp_sensor = nullptr;
    sensor::Sensor *water_flow_sensor = nullptr;
    sensor::Sensor *water_utilization_sensor = nullptr;
    sensor::Sensor *gas_total_sensor = nullptr;
    sensor::Sensor *gas_current_sensor = nullptr;
    sensor::Sensor *sh_set_temp_sensor = nullptr;
    sensor::Sensor *sh_outlet_temp_sensor = nullptr;
    sensor::Sensor *sh_return_temp_sensor = nullptr;
    sensor::Sensor *outdoor_temp_sensor = nullptr;
    sensor::Sensor *heat_capacity_sensor = nullptr;
    sensor::Sensor *total_dhw_usage_sensor = nullptr;
    sensor::Sensor *total_operating_time_sensor = nullptr;
    sensor::Sensor *cumulative_dwh_usage_hours_sensor = nullptr;
    sensor::Sensor *cumulative_sh_usage_hours_sensor = nullptr;
    sensor::Sensor *cumulative_domestic_usage_cnt_sensor = nullptr;
    sensor::Sensor *days_since_install_sensor = nullptr;

    text_sensor::TextSensor *controller_version_sensor = nullptr;
    text_sensor::TextSensor *panel_version_sensor = nullptr;
    text_sensor::TextSensor *heating_mode_sensor = nullptr;
    text_sensor::TextSensor *device_type_sensor = nullptr;
    text_sensor::TextSensor *operating_state_sensor = nullptr;
    text_sensor::TextSensor *recirc_mode_sensor = nullptr;

    binary_sensor::BinarySensor *boiler_active_sensor = nullptr;
    binary_sensor::BinarySensor *conn_status_sensor = nullptr;
    binary_sensor::BinarySensor *recirc_running_sensor = nullptr;
    binary_sensor::BinarySensor *other_navilink_installed_sensor = nullptr;

#ifdef USE_SWITCH
    switch_::Switch *power_switch = nullptr;
    switch_::Switch *allow_recirc_switch = nullptr;
#endif

#ifdef USE_CLIMATE
    climate::Climate *climate = nullptr;
#endif

#ifdef USE_WATER_HEATER
    NavienWaterHeater *water_heater = nullptr;
#endif

    NavienLink *navien_link_;
    esphome::uart::UARTComponent* uart_;
    uint8_t src_;
    bool is_rt;
  };

  class Navien : public PollingComponent, public NavienBase {
  public:
    Navien() {
      updated_cnt = 0;
      received_cnt = 0;
      is_connected = false;
      navien_link_ = nullptr;
    }

    virtual float get_setup_priority() const { return setup_priority::HARDWARE; }
    virtual void setup() override;
    void loop() override;
    void update() override;
    void dump_config() override;

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

    /**
     * Helper function to convert operating state enum to string
     */
    static std::string op_state_to_str(OPERATING_STATE state);

    /**
     * Helper function to convert heating mode enum to string
     */
    static std::string heat_mode_to_str(DEVICE_HEATING_MODE mode);

    /**
     * Helper function to convert device type enum to string
     */
    static std::string device_type_to_str(DEVICE_TYPE type);

    /**
     * Helper function to convert recirculation mode enum to string
     */
    static std::string device_recirc_mode_to_str(DEVICE_RECIRC_MODE state);

  protected:
    // Data, extracted from gas and water packers and stored
    // Once the "update" is called this data gets reported to readers.
    NAVIEN_STATE state = {};

  protected:
    /**
     * NavienLinkVisitorI interface implementation
     */
    virtual void on_water(const WATER_DATA & water, uint8_t src);
    virtual void on_gas(const GAS_DATA & gas, uint8_t src);
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
class NavienOnOffSwitch : public switch_::Switch, public Component {
  protected:
    Navien * parent = nullptr;
  public:
    void setup() override;

    void set_parent(Navien * parent);
    void write_state(bool state) override;
    void dump_config() override;
  };

  class NavienAllowScheduledRecircSwitch : public switch_::Switch, public Component {
  protected:
    Navien * parent = nullptr;
  public:
    void setup() override;

    void set_parent(Navien * parent);
    void write_state(bool state) override;
    void dump_config() override;
  };
#endif

#ifdef USE_BUTTON
class NavienHotButton : public button::Button, public Component {
  protected:
    Navien * parent = nullptr;
  public:
    void setup() override;

    void set_parent(Navien * parent);

    void press_action() override;
    void dump_config() override;
  };
#endif

}  // namespace navien
}  // namespace esphome
