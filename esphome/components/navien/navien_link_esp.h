#pragma once

#include <cinttypes>
#include <cstring>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include "navien_link.h"

namespace esphome {
namespace navien {

static const uint8_t NAVIEN_CASCADE_MAX = 16;

/**
 * ESPHome-specific implementation of NavienUartI interface.
 * This class bridges the NavienLink protocol handler with ESPHome's UART component.
 * It acts as a singleton that broadcasts received packets to all registered visitors.
 * Inherits from Component to participate in ESPHome's loop() mechanism.
 */
class NavienLinkEsp : public Component, public uart::UARTDevice, protected NavienUartI, public NavienLinkVisitorI {
public:
  NavienLinkEsp() : navien_link(*this, *this) {
    memset(visitors_, 0, sizeof(visitors_));
  }

  /**
   * Register a visitor to receive callbacks when packets are received.
   * @param visitor - pointer to the visitor instance
   * @param src - index in the visitor array (0-15), defaults to 0
   */
  void add_visitor(NavienLinkVisitorI *visitor, uint8_t src = 0) { 
    if (visitor != nullptr && src < NAVIEN_CASCADE_MAX) {
      visitors_[src] = visitor; 
    }
  }

  /**
   * Send commands to Navien unit
   */
  void send_turn_on_cmd() { this->navien_link.send_turn_on_cmd(); }
  void send_turn_off_cmd() { this->navien_link.send_turn_off_cmd(); }
  void send_hot_button_cmd() { this->navien_link.send_hot_button_cmd(); }
  void send_set_temp_cmd(float temp) { this->navien_link.send_set_temp_cmd(temp); }
  void send_scheduled_recirculation_on_cmd() { this->navien_link.send_scheduled_recirculation_on_cmd(); }
  void send_scheduled_recirculation_off_cmd() { this->navien_link.send_scheduled_recirculation_off_cmd(); }
  bool is_other_navilink_installed() { return this->navien_link.is_other_navilink_installed(); }

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
   * NavienLinkVisitorI interface implementation - calls the visitor for the specific src
   */
  virtual void on_water(const WATER_DATA &water, uint8_t src) override {
    ESP_LOGD("navien_link_esp", "SRC:0x%02X on_water called", src);
    NavienLinkVisitorI *visitor = get_visitor(src);
    if (visitor != nullptr) {
      visitor->on_water(water, src);
    }
  }

  virtual void on_gas(const GAS_DATA &gas, uint8_t src) override {
    ESP_LOGD("navien_link_esp", "SRC:0x%02X on_gas called", src);
    NavienLinkVisitorI *visitor = get_visitor(src);
    if (visitor != nullptr) {
      visitor->on_gas(gas, src);
    }
  }

  virtual void on_error() override {
    for (uint8_t i = 0; i < NAVIEN_CASCADE_MAX; i++) {
      if (visitors_[i] != nullptr) {
        visitors_[i]->on_error();
      }
    }
  }

protected:
  /**
   * Get visitor by src address.
   * @param src - the source address from the packet header (PACKET_SRC_STATUS+)
   * @return pointer to the visitor or nullptr if out of bounds
   */
  NavienLinkVisitorI* get_visitor(uint8_t src) {
    uint8_t idx = src - PACKET_SRC_STATUS;
    if (idx < NAVIEN_CASCADE_MAX) {
      return visitors_[idx];
    }
    return nullptr;
  }

  NavienLink navien_link;
  NavienLinkVisitorI *visitors_[NAVIEN_CASCADE_MAX];
};

}  // namespace navien
}  // namespace esphome
