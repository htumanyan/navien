#include "esphome.h"
#include "navien_climate.h"

namespace esphome {
namespace navien {

static const char *TAG = "navien.climate";

void NavienClimate::setup() {
}

void NavienClimate::dump_config() {
}

climate::ClimateTraits NavienClimate::traits() {
    auto traits = climate::ClimateTraits();

    // This is deprecated but still works (just a warning).
    // You can comment it out if the warning bothers you.
    traits.set_supports_current_temperature(true);

    // OLD (caused compile error):
    // static const std::set<climate::ClimateMode> supported_modes =
    //     {climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT};
    // traits.set_supported_modes(supported_modes);

    // NEW: tell ESPHome which modes are supported one-by-one
    traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);

    return traits;
}

void NavienClimate::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {

    esphome::optional<float> f = call.get_target_temperature();
    float target = *f;

    ESP_LOGD(TAG, "Setting target temperature to %f", target);
    parent->send_set_temp_cmd(target);
  }

  if (call.get_mode().has_value()) {
    climate::ClimateMode mode = call.get_mode().value();
    ESP_LOGD(TAG, "Setting mode to %s", climate::climate_mode_to_string(mode));
    switch (mode) {
      case climate::ClimateMode::CLIMATE_MODE_OFF:
        parent->send_turn_off_cmd();
        break;
      case climate::ClimateMode::CLIMATE_MODE_HEAT:
        parent->send_turn_on_cmd();
        break;
      default:
        ESP_LOGD(TAG, "Unsupported Mode (%s)", climate::climate_mode_to_string(mode));
    }
  }
}

void NavienClimate::set_parent(Navien *parent_) {
  this->parent = parent_;
  parent->set_climate(this);
}

}  // namespace navien
}  // namespace esphome
