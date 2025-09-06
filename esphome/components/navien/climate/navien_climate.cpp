

#include "esphome.h"
#include "navien_climate.h"

namespace esphome {
namespace navien {
  
static const char *TAG = "navien.climate";
static const std::set<climate::ClimateMode> supported_modes = {climate::CLIMATE_MODE_HEAT};
  
  void NavienClimate::setup(){
  }
  
  void NavienClimate::dump_config(){
  }


  climate::ClimateTraits NavienClimate::traits(){
    auto traits = climate::ClimateTraits();
    
    traits.set_supports_current_temperature(true);
    traits.set_supported_modes(supported_modes);
    return traits;
  }
  
  void NavienClimate::control(const climate::ClimateCall &call){
    if (!call.get_target_temperature().has_value())
      return;

    esphome::optional<float> f = call.get_target_temperature();
    float target = (*f - 32.f) * 5 / 9;
    
    ESP_LOGE(TAG, "Setting target temperature to %f", target);
    parent->send_set_temp_cmd(target);
  }

  void NavienClimate::set_parent(Navien * parent_){
    this->parent = parent_;
    parent->set_climate(this);
  }
}
}
