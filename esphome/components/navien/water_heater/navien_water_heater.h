#pragma once

#include "esphome/components/water_heater/water_heater.h"

namespace esphome {
namespace navien {

class NavienBase;


// WaterHeater implementation for DHW control
class NavienWaterHeater : public water_heater::WaterHeater, public Component {
public:
  NavienWaterHeater();
  void set_parent(NavienBase * parent_);

  virtual void setup() override;
  virtual void dump_config() override;
  virtual water_heater::WaterHeaterCallInternal make_call() override;
  virtual void control(const water_heater::WaterHeaterCall &call) override;

  void set_min_temperature(float t) { min_temperature = t; }
  void set_max_temperature(float t) { max_temperature = t; }

  // Most base class setters are protected. Wrap and make public.
  // esphome side state only, use control() for actual changes.
  void set_on_state(bool is_on);
  void set_target_temperature_state(float t);

protected:
  virtual water_heater::WaterHeaterTraits traits() override;

private:
  NavienBase * parent = nullptr;

  // Conservative safe defaults, changeable via config
  float min_temperature = 32.22; // 90F
  float max_temperature = 48.89; // 120F
};

} // navien
} // esphome
