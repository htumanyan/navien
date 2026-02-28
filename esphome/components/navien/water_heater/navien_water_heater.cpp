#include "navien_water_heater.h"
#include "../navien.h"

namespace esphome {
namespace navien {

using namespace water_heater;

static const char *TAG = "navien.weater_heater";

NavienWaterHeater::NavienWaterHeater() {
}

void NavienWaterHeater::set_parent(NavienBase * parent_) {
  this->parent = parent_;
  this->parent->set_water_heater(this);
}

void NavienWaterHeater::setup() {
}

void NavienWaterHeater::dump_config() {
  ESP_LOGCONFIG(TAG, "NavienWaterHeater:\n"
                     "  name: %s\n"
                     "  min_temperature: %.1f\n"
                     "  max_temperature: %.1f",
                     this->get_name().c_str(),
                     this->min_temperature,
                     this->max_temperature);
}

WaterHeaterCallInternal NavienWaterHeater::make_call() {
  return WaterHeaterCallInternal(this);
}

void NavienWaterHeater::control(const WaterHeaterCall &call) {
  if (parent == nullptr) {
    ESP_LOGE(TAG, "No Navien parent set");
  }
  
  bool set_on_to = is_on();

  if (call.get_mode().has_value()) {
    set_on_to = (*call.get_mode() != WATER_HEATER_MODE_OFF);
  }

  if (call.get_on().has_value()) {
    set_on_to = *call.get_on();
  }

  if (set_on_to != is_on()) {
    if (set_on_to) {
      parent->send_turn_on_cmd();
    } else {
      parent->send_turn_off_cmd();
    }
  }

  if (!std::isnan(call.get_target_temperature())) {
    parent->send_dhw_set_temp_cmd(call.get_target_temperature());
  }

  // Away setting not supported
  //if (call.get_away().has_value()) {
  //  *call.get_away();
  //}
}

void NavienWaterHeater::set_on_state(bool is_on) {
  this->set_state_flag_(WATER_HEATER_STATE_ON, is_on);
  if (is_on) {
    this->set_mode_(WaterHeaterMode::WATER_HEATER_MODE_GAS);
  } else {
    this->set_mode_(WaterHeaterMode::WATER_HEATER_MODE_OFF);
  }
}

void NavienWaterHeater::set_target_temperature_state(float t) {
  this->set_target_temperature_(t);
}


WaterHeaterTraits NavienWaterHeater::traits() {
  WaterHeaterTraits traits;
  // Do any models have other settings (eco, high performance, etc)?
  traits.set_supported_modes({
    WaterHeaterMode::WATER_HEATER_MODE_OFF,
    WaterHeaterMode::WATER_HEATER_MODE_GAS
  });
  traits.add_feature_flags(WATER_HEATER_SUPPORTS_OPERATION_MODE);

  traits.add_feature_flags(WATER_HEATER_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(WATER_HEATER_SUPPORTS_TARGET_TEMPERATURE);
  traits.add_feature_flags(WATER_HEATER_SUPPORTS_ON_OFF);
  
  // Do any models have an away mode or are there commands for schedule control?
  //traits.add_feature_flags(WATER_HEATER_SUPPORTS_AWAY_MODE);

  traits.set_min_temperature(this->min_temperature);
  traits.set_max_temperature(this->max_temperature);

  return traits;
}

} // navien
} // esphome
