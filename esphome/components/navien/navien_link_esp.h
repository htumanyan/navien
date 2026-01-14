#pragma once

#include <cinttypes>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "navien_link.h"

namespace esphome {
namespace navien {

/**
 * ESPHome-specific implementation of NavienUartI interface.
 * This class bridges the NavienLink protocol handler with ESPHome's UART component.
 * It acts as a singleton that broadcasts received packets to all registered visitors.
 * Inherits from Component to participate in ESPHome's loop() mechanism.
 */
class NavienLinkEsp : public Component, public uart::UARTDevice, protected NavienUartI, public NavienLinkVisitorI {
public:
  NavienLinkEsp() : navien_link(*this, *this) {}

  /**
   * Register a visitor to receive callbacks when packets are received.
   * Multiple visitors can be registered to receive the same data.
   */
  void add_visitor(NavienLinkVisitorI *visitor) { 
    if (visitor != nullptr) {
      visitors_.push_back(visitor); 
    }
  }

  /**
   * Send commands to Navien unit
   */
  void send_turn_on_cmd() { this->navien_link.send_turn_on_cmd(); }
  void send_turn_off_cmd() { this->navien_link.send_turn_off_cmd(); }
  void send_hot_button_cmd() { this->navien_link.send_hot_button_cmd(); }
  void send_set_temp_cmd(float temp) { this->navien_link.send_set_temp_cmd(temp); }

  /**
   * Component interface - called in the main loop
   */
  void loop() override { this->navien_link.receive(); }
  
  float get_setup_priority() const override { return setup_priority::DATA; }

protected:
  /**
   * NavienUartI interface implementation for ESPHome
   */
  virtual int available() override { return uart::UARTDevice::available(); }
  virtual uint8_t peek_byte(uint8_t *byte) override { return uart::UARTDevice::peek_byte(byte); }
  virtual uint8_t read_byte(uint8_t *byte) override { return uart::UARTDevice::read_byte(byte); }
  virtual bool read_array(uint8_t *data, uint8_t len) override { return uart::UARTDevice::read_array(data, len); }
  virtual void write_array(const uint8_t *data, uint8_t len) override { uart::UARTDevice::write_array(data, len); }

  /**
   * NavienLinkVisitorI interface implementation - broadcasts to all registered visitors
   */
  virtual void on_water(const WATER_DATA &water) override {
    for (auto *visitor : visitors_) {
      visitor->on_water(water);
    }
  }

  virtual void on_gas(const GAS_DATA &gas) override {
    for (auto *visitor : visitors_) {
      visitor->on_gas(gas);
    }
  }

  virtual void on_error() override {
    for (auto *visitor : visitors_) {
      visitor->on_error();
    }
  }

protected:
  NavienLink navien_link;
  std::vector<NavienLinkVisitorI *> visitors_;
};

}  // namespace navien
}  // namespace esphome
