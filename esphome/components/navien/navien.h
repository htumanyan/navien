#pragma once

#include <cinttypes>
#include <list>

#include "esphome/core/component.h"
//#ifdef USE_SENSOR
    #include "esphome/components/sensor/sensor.h"
//#endif

//#ifdef USE_BINARY_SENSOR
    #include "esphome/components/binary_sensor/binary_sensor.h"
//#endif

//#ifdef USE_SWITCH
    #include "esphome/components/switch/switch.h"
//#endif
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"

#include "navien_link.h"

#ifndef USE_SWITCH
namespace switch_ {
class Switch;
}
#endif

namespace esphome {
namespace navien {


typedef enum _DEVICE_POWER_STATE{
  POWER_OFF,
  POWER_ON
} DEVICE_POWER_STATE;


typedef enum _DEVICE_RECIRC_STATE{
  RECIRCULATION_OFF,
  RECIRCULATION_ON
} DEVICE_RECIRC_STATE;

typedef enum _DEVICE_UNITS{
  CELSIUS,
  FARENHEIT
} DEVICE_UNITS;
  
typedef struct{
  struct{
    uint8_t set_temp;
    uint8_t outlet_temp;
    uint8_t inlet_temp;
    float flow_lpm;
    uint8_t utilization;
    uint8_t sys_status;
  } water;
  struct{
    uint8_t  set_temp;
    uint8_t  outlet_temp;
    uint8_t  inlet_temp;
    uint16_t accumulated_gas_usage;
    float    accumulated_gas_usage_cuft;
    uint16_t current_gas_usage;

  } gas;

  uint16_t           controller_version;
  uint16_t           panel_version;
  DEVICE_POWER_STATE  power;
  DEVICE_RECIRC_STATE recirculation;
  DEVICE_UNITS        units;
} NAVIEN_STATE;
  

class NavienBase : public uart::UARTDevice, protected NavienUartI, public NavienLinkVisitorI{
public:
  NavienBase() : navien_link(*this, *this), is_rt(false){}

public:
  void set_target_temp_sensor(sensor::Sensor *sensor) { target_temp_sensor = sensor; }
  void set_inlet_temp_sensor(sensor::Sensor *sensor) { inlet_temp_sensor = sensor; }
  void set_outlet_temp_sensor(sensor::Sensor *sensor) { outlet_temp_sensor = sensor; }
  void set_water_flow_sensor(sensor::Sensor *sensor) { water_flow_sensor = sensor; }
  void set_water_utilization_sensor(sensor::Sensor *sensor) { water_utilization_sensor = sensor; }
  void set_sys_status_sensor(sensor::Sensor *sensor) { sys_status_sensor = sensor; }
  void set_gas_total_sensor(sensor::Sensor *sensor) { gas_total_sensor = sensor; }
  void set_gas_total_cuft_sensor(sensor::Sensor *sensor) { gas_total_cuft_sensor = sensor; }
  void set_gas_current_sensor(sensor::Sensor *sensor) { gas_current_sensor = sensor; }
  void set_real_time(bool rt){this->is_rt = rt;}

  void set_conn_status_sensor(binary_sensor::BinarySensor *sensor) { conn_status_sensor = sensor; }
  void set_recirc_status_sensor(binary_sensor::BinarySensor *sensor) { recirc_status_sensor = sensor; }  

  /**
   * Sets the power switch component that will be notified of power
   * state changes, i.e. if the unit goes off, Navien will updte the switch to reflect that
   */
  void set_power_switch(switch_::Switch * ps){power_switch = ps;}


  void set_climate(climate::Climate * c){climate = c;}

  /**
   * Sets the climate component that will be receiving temperature updates
   */
  //void set_climate(switch_::Switch * ps){power_switch = ps;}


  void send_turn_on_cmd(){this->navien_link.send_turn_on_cmd();}
  void send_turn_off_cmd(){this->navien_link.send_turn_off_cmd();}
  void send_hot_button_cmd(){this->navien_link.send_hot_button_cmd();}
  void send_set_temp_cmd(float temp){this->navien_link.send_set_temp_cmd(temp);}
  
protected:
  /**
   * NavienUartI interface implementation for ESPHome
   */
  virtual int     available() {return uart::UARTDevice::available();}
  virtual uint8_t peek_byte(uint8_t * byte){return uart::UARTDevice::peek_byte(byte);}
  virtual uint8_t read_byte(uint8_t * byte){return uart::UARTDevice::read_byte(byte);}
  virtual bool read_array(uint8_t * data, uint8_t len){return uart::UARTDevice::read_array(data, len);}
  virtual void write_array(const uint8_t * data, uint8_t len){uart::UARTDevice::write_array(data, len);}

  
protected:
  NavienLink navien_link;

  /**
   * Sensor definitions
   */

  sensor::Sensor *target_temp_sensor = nullptr;
  sensor::Sensor *outlet_temp_sensor = nullptr;
  sensor::Sensor *inlet_temp_sensor = nullptr;
  sensor::Sensor *water_flow_sensor = nullptr;
  sensor::Sensor *water_utilization_sensor = nullptr;
  sensor::Sensor *sys_status_sensor = nullptr;
  sensor::Sensor *gas_total_sensor = nullptr;
  sensor::Sensor *gas_total_cuft_sensor = nullptr;
  sensor::Sensor *gas_current_sensor = nullptr;


  binary_sensor::BinarySensor *conn_status_sensor = nullptr;
  binary_sensor::BinarySensor *recirc_status_sensor = nullptr;
  
  switch_::Switch *power_switch = nullptr;
  climate::Climate *climate = nullptr;

  bool is_rt;
};
		   
class Navien : public PollingComponent, public NavienBase {
public:
  Navien() {
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
  void loop() override {
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
  virtual void on_water(const WATER_DATA & water);
  virtual void on_gas(const GAS_DATA & gas);
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
