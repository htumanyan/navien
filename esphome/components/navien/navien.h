#pragma once

#include <cinttypes>
#include <list>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#include "esphome/components/uart/uart.h"

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
  
typedef struct{
  struct{
    uint8_t set_temp;
    uint8_t outlet_temp;
    uint8_t inlet_temp;
    float flow_lpm;
    uint8_t utilization;
  } water;
  struct{
    uint8_t  set_temp;
    uint8_t  outlet_temp;
    uint8_t  inlet_temp;
    uint16_t accumulated_gas_usage;
    uint16_t current_gas_usage;
  } gas;

  uint16_t           controller_version;
  uint16_t           panel_version;
  DEVICE_POWER_STATE power;
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
  void set_gas_total_sensor(sensor::Sensor *sensor) { gas_total_sensor = sensor; }
  void set_gas_current_sensor(sensor::Sensor *sensor) { gas_current_sensor = sensor; }  
  void set_real_time(bool rt){this->is_rt = rt;}
  
  void set_power_switch(switch_::Switch * ps){power_switch = ps;}


  void send_turn_on_cmd(){this->navien_link.send_turn_on_cmd();}
  void send_turn_off_cmd(){this->navien_link.send_turn_off_cmd();}
  void send_hot_button_cmd(){this->navien_link.send_hot_button_cmd();}
  void send_set_temp_cmd(){this->navien_link.send_set_temp_cmd();}
  
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
  sensor::Sensor *target_temp_sensor;
  sensor::Sensor *outlet_temp_sensor;
  sensor::Sensor *inlet_temp_sensor;
  sensor::Sensor *water_flow_sensor;
  sensor::Sensor *water_utilization_sensor;
  sensor::Sensor *gas_total_sensor;
  sensor::Sensor *gas_current_sensor;

  switch_::Switch *power_switch;
  bool is_rt;
};
		   
class Navien : public PollingComponent, public NavienBase {
public:
  Navien() {
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
  // Data, extracted from gas and water packers and stored
  // Once the "update" is called this data gets reported to readers.
  NAVIEN_STATE state;

protected:
  /**
   * NavienLinkVisitorI interface implementation
   */
  virtual void on_water(const WATER_DATA & water);
  virtual void on_gas(const GAS_DATA & gas);
  virtual void on_error();
};

class NavienRT : public Component, public NavienBase {
public:
  NavienRT(){
  }
  
public:
  /**
   * NavienLinkVisitorI interface implementation
   */
  virtual void on_water(const WATER_DATA & water);
  virtual void on_gas(const GAS_DATA & gas);
  virtual void on_error();
};

#ifdef USE_SWITCH
class NavienOnOffSwitch : public switch_::Switch, public Component {
    protected:
      Navien * parent;
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
      Navien * parent;
    public:
      void setup() override;

      void set_parent(Navien * parent);

      void press_action() override;
      void dump_config() override;
};
#endif
    
}  // namespace navien
}  // namespace esphome
