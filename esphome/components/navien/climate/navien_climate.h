#pragma once

#include "esphome/components/climate/climate.h"
#include "../navien.h"

namespace esphome {
namespace navien {

class NavienClimate : public climate::Climate, public Component {
public:
  NavienClimate() {
    mode = climate::CLIMATE_MODE_HEAT;

  }
protected:
	
  NavienBase * parent = nullptr;
public:
  void setup() override;
  void dump_config() override;

  void set_parent(Navien * parent_);

  /**
   * Abstract methods of the Climate class that must be overriden
   */
  virtual climate::ClimateTraits traits();
  virtual void control(const climate::ClimateCall &call);
};

} // esphome
} // navien
