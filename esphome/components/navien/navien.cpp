#include <cmath>
#include <string>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"

namespace esphome
{
  namespace navien
  {

    static const char *TAG = "navien.sensor";

    void Navien::setup()
    {
      this->state.power = POWER_OFF;
    }

    void Navien::on_water(const WATER_DATA &water)
    {
      ESP_LOGV(TAG, "Received Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, Flow: 0x%02X, Sys Power: 0x%02X, Sys Status: 0x%02X, Recirc Enabled: 0x%02X",
               water.set_temp,
               water.inlet_temp,
               water.outlet_temp,
               water.water_flow,
               water.system_power,
               water.system_status,
               water.recirculation_enabled);
      ESP_LOGV(TAG, "Received WATER: unk_06: 0x%02X, unk_07: 0x%02X, unk_10: 0x%02X, unk_14: 0x%02X, unk_15: 0x%02X, unk_17: 0x%02X, unk_20: 0x%02X, unk_21: 0x%02X, unk_22: 0x%02X, unk_23: 0x%02X, unk_24: 0x%02X, unk_25: 0x%02X, unk_26: 0x%02X, unk_27: 0x%02X, unk_28: 0x%02X, unk_29: 0x%02X, unk_30: 0x%02X, unk_31: 0x%02X, unk_32: 0x%02X, unk_34: 0x%02X, unk_35: 0x%02X, unk_36: 0x%02X, unk_37: 0x%02X, unk_38: 0x%02X, unk_39: 0x%02X",
               water.unknown_06, water.unknown_07, water.unknown_10, water.unknown_14,
               water.unknown_15, water.unknown_17, water.unknown_20, water.unknown_21, water.unknown_22,
               water.unknown_23, water.unknown_24, water.unknown_25, water.unknown_26, water.unknown_27,
               water.unknown_28, water.unknown_29, water.unknown_30, water.unknown_31, water.unknown_32,
               water.unknown_34, water.unknown_35, water.unknown_36, water.unknown_37, water.unknown_38, water.unknown_39);

      if (water.system_power & POWER_STATUS_ON_OFF_MASK)
      {
        state.power = POWER_ON;
      }
      else
      {
        state.power = POWER_OFF;
      }

      if (water.system_status & SYS_STATUS_FLAG_RECIRC)
      {
        state.recirculation = RECIRCULATION_ON;
      }
      else
      {
        state.recirculation = RECIRCULATION_OFF;
      }

      if (water.system_status & SYS_STATUS_FLAG_UNITS)
      {
        state.units = CELSIUS;
      }
      else
      {
        state.units = FARENHEIT;
      }

      if (water.combi_mode & COMBI_MODE_SPACE_HEATING_MASK)
      {
        state.combi_mode = COMBI_MODE_SPACE_HEATING;
      }
      else if (water.combi_mode & COMBI_MODE_DOMESTIC_HOT_WATER_MASK)
      {
        state.combi_mode = COMBI_MODE_DOMESTIC_HOT_WATER;
      }
      else
      {
        state.combi_mode = COMBI_MODE_IDLE;
      }

      // Update the counter that will be used in assessment
      // of whether we're connected to navien or not
      this->received_cnt++;

      this->state.water.set_temp = NavienLink::t2c(water.set_temp);
      this->state.water.outlet_temp = NavienLink::t2c(water.outlet_temp);
      this->state.water.inlet_temp = NavienLink::t2c(water.inlet_temp);
      this->state.water.flow_lpm = NavienLink::flow2lpm(water.water_flow);
      this->state.water.utilization = water.operating_capacity * 0.5f;
      this->state.water_unknowns.unknown_06 = water.unknown_06;
      this->state.water_unknowns.unknown_07 = water.unknown_07;
      this->state.water_unknowns.unknown_10 = water.unknown_10;
      this->state.water_unknowns.unknown_14 = water.unknown_14;
      this->state.water_unknowns.unknown_15 = water.unknown_15;
      this->state.water_unknowns.unknown_17 = water.unknown_17;
      this->state.water_unknowns.unknown_20 = water.unknown_20;
      this->state.water_unknowns.unknown_21 = water.unknown_21;
      this->state.water_unknowns.unknown_22 = water.unknown_22;
      this->state.water_unknowns.unknown_23 = water.unknown_23;
      this->state.water_unknowns.unknown_24 = water.unknown_24;
      this->state.water_unknowns.unknown_25 = water.unknown_25;
      this->state.water_unknowns.unknown_26 = water.unknown_26;
      this->state.water_unknowns.unknown_27 = water.unknown_27;
      this->state.water_unknowns.unknown_28 = water.unknown_28;
      this->state.water_unknowns.unknown_29 = water.unknown_29;
      this->state.water_unknowns.unknown_30 = water.unknown_30;
      this->state.water_unknowns.unknown_31 = water.unknown_31;
      this->state.water_unknowns.unknown_32 = water.unknown_32;
      this->state.water_unknowns.unknown_34 = water.unknown_34;
      this->state.water_unknowns.unknown_35 = water.unknown_35;
      this->state.water_unknowns.unknown_36 = water.unknown_36;
      this->state.water_unknowns.unknown_37 = water.unknown_37;
      this->state.water_unknowns.unknown_38 = water.unknown_38;
      this->state.water_unknowns.unknown_39 = water.unknown_39;
      if (this->is_rt)
        this->update_water_sensors();
    }

    void Navien::on_gas(const GAS_DATA &gas)
    {
      ESP_LOGV(TAG, "Received Gas Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X",
               gas.set_temp,
               gas.inlet_temp,
               gas.outlet_temp);

      ESP_LOGV(TAG, "Received Accumulated: 0x%02X 0x%02X, Current Gas: 0x%02X 0x%02X, Unk_19: 0x%02X",
               gas.cumulative_gas_hi,
               gas.cumulative_gas_lo,
               gas.current_gas_hi,
               gas.current_gas_lo,
               gas.unknown_19);
      ESP_LOGV(TAG, "Received GAS: unk_00: 0x%02X, unk_01: 0x%02X, unk_02: 0x%02X, unk_03: 0x%02X, unk_18: 0x%02X, unk_19: 0x%02X, unk_20: 0x%02X",
               gas.unknown_00, gas.unknown_01, gas.unknown_02, gas.unknown_03,
               gas.unknown_18, gas.unknown_19, gas.unknown_20);
      // Update the counter that will be used in assessment
      // of whether we're connected to navien or not
      this->received_cnt++;

      this->state.gas.set_temp = NavienLink::t2c(gas.set_temp);
      this->state.gas.outlet_temp = NavienLink::t2c(gas.outlet_temp);
      this->state.gas.inlet_temp = NavienLink::t2c(gas.inlet_temp);
      this->state.gas.sh_outlet_temp = NavienLink::t2c(gas.sh_outlet_temp);
      this->state.gas.sh_return_temp = NavienLink::t2c(gas.sh_return_temp);
      this->state.gas_unknowns.unknown_00 = gas.unknown_00;
      this->state.gas_unknowns.unknown_01 = gas.unknown_01;
      this->state.gas_unknowns.unknown_02 = gas.unknown_02;
      this->state.gas_unknowns.unknown_03 = gas.unknown_03;
      this->state.gas_unknowns.unknown_18 = gas.unknown_18;
      this->state.gas_unknowns.unknown_19 = gas.unknown_19;
      this->state.gas_unknowns.unknown_20 = gas.unknown_20;

      this->state.gas.accumulated_gas_usage = gas.cumulative_gas_hi << 8 | gas.cumulative_gas_lo;
      this->state.gas.current_gas_usage = gas.current_gas_hi << 8 | gas.current_gas_lo;

      this->state.controller_version = gas.controller_version_hi << 8 | gas.controller_version_lo;
      this->state.panel_version = gas.panel_version_hi << 8 | gas.panel_version_lo;

      if (this->is_rt)
        this->update_gas_sensors();
    }

    void Navien::on_error()
    {
      this->target_temp_sensor->publish_state(0);
      this->outlet_temp_sensor->publish_state(0);
      this->inlet_temp_sensor->publish_state(0);
      this->water_flow_sensor->publish_state(0);
      this->sh_outlet_temp_sensor->publish_state(0);
      this->sh_return_temp_sensor->publish_state(0);
      this->is_connected = false;
    }

    void Navien::update_water_sensors()
    {
      if (this->water_flow_sensor != nullptr)
        this->water_flow_sensor->publish_state(this->state.water.flow_lpm);

      if (this->water_utilization_sensor != nullptr)
        this->water_utilization_sensor->publish_state(this->state.water.utilization);
      if (this->combi_mode_sensor != nullptr) {
        std::string combi_mode_str;
        switch (this->state.combi_mode) {
          case COMBI_MODE_IDLE:
            combi_mode_str = "Idle";
            break;
          case COMBI_MODE_SPACE_HEATING:
            combi_mode_str = "Space Heating";
            break;
          case COMBI_MODE_DOMESTIC_HOT_WATER:
            combi_mode_str = "Domestic Hot Water";
            break;
        }
        this->combi_mode_sensor->publish_state(combi_mode_str);
      }
      if (this->unk_sensor_w06 != nullptr)
        this->unk_sensor_w06->publish_state(this->state.water_unknowns.unknown_06);
      if (this->unk_sensor_w07 != nullptr)
        this->unk_sensor_w07->publish_state(this->state.water_unknowns.unknown_07);
      if (this->unk_sensor_w10 != nullptr)
        this->unk_sensor_w10->publish_state(this->state.water_unknowns.unknown_10);
      if (this->unk_sensor_w14 != nullptr)
        this->unk_sensor_w14->publish_state(this->state.water_unknowns.unknown_14);
      if (this->unk_sensor_w15 != nullptr)
        this->unk_sensor_w15->publish_state(this->state.water_unknowns.unknown_15);
      if (this->unk_sensor_w17 != nullptr)
        this->unk_sensor_w17->publish_state(this->state.water_unknowns.unknown_17);
      if (this->unk_sensor_w20 != nullptr)
        this->unk_sensor_w20->publish_state(this->state.water_unknowns.unknown_20);
      if (this->unk_sensor_w21 != nullptr)
        this->unk_sensor_w21->publish_state(this->state.water_unknowns.unknown_21);
      if (this->unk_sensor_w22 != nullptr)
        this->unk_sensor_w22->publish_state(this->state.water_unknowns.unknown_22);
      if (this->unk_sensor_w23 != nullptr)
        this->unk_sensor_w23->publish_state(this->state.water_unknowns.unknown_23);
      if (this->unk_sensor_w24 != nullptr)
        this->unk_sensor_w24->publish_state(this->state.water_unknowns.unknown_24);
      if (this->unk_sensor_w25 != nullptr)
        this->unk_sensor_w25->publish_state(this->state.water_unknowns.unknown_25);
      if (this->unk_sensor_w26 != nullptr)
        this->unk_sensor_w26->publish_state(this->state.water_unknowns.unknown_26);
      if (this->unk_sensor_w27 != nullptr)
        this->unk_sensor_w27->publish_state(this->state.water_unknowns.unknown_27);
      if (this->unk_sensor_w28 != nullptr)
        this->unk_sensor_w28->publish_state(this->state.water_unknowns.unknown_28);
      if (this->unk_sensor_w29 != nullptr)
        this->unk_sensor_w29->publish_state(this->state.water_unknowns.unknown_29);
      if (this->unk_sensor_w30 != nullptr)
        this->unk_sensor_w30->publish_state(this->state.water_unknowns.unknown_30);
      if (this->unk_sensor_w31 != nullptr)
        this->unk_sensor_w31->publish_state(this->state.water_unknowns.unknown_31);
      if (this->unk_sensor_w32 != nullptr)
        this->unk_sensor_w32->publish_state(this->state.water_unknowns.unknown_32);
      if (this->unk_sensor_w34 != nullptr)
        this->unk_sensor_w34->publish_state(this->state.water_unknowns.unknown_34);
      if (this->unk_sensor_w35 != nullptr)
        this->unk_sensor_w35->publish_state(this->state.water_unknowns.unknown_35);
      if (this->unk_sensor_w36 != nullptr)
        this->unk_sensor_w36->publish_state(this->state.water_unknowns.unknown_36);
      if (this->unk_sensor_w37 != nullptr)
        this->unk_sensor_w37->publish_state(this->state.water_unknowns.unknown_37);
      if (this->unk_sensor_w38 != nullptr)
        this->unk_sensor_w38->publish_state(this->state.water_unknowns.unknown_38);
      if (this->unk_sensor_w39 != nullptr)
        this->unk_sensor_w39->publish_state(this->state.water_unknowns.unknown_39);

      // Update the climate control with the current target temperature
      if (this->climate != nullptr)
      {
        this->climate->current_temperature = this->state.water.outlet_temp;
        this->climate->target_temperature = this->state.water.set_temp;
        this->climate->publish_state();
      }

      if (this->recirc_status_sensor != nullptr)
      {
        switch (this->state.recirculation)
        {
        case RECIRCULATION_ON:
          this->recirc_status_sensor->publish_state(true);
          break;
        default:
          this->recirc_status_sensor->publish_state(false);
        }
      }

      switch (this->state.power)
      {
      case POWER_ON:
        if (this->power_switch != nullptr)
          this->power_switch->publish_state(true);
        if (this->climate != nullptr)
        {
          this->climate->mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
          this->climate->publish_state();
        }
        break;
      default:
        if (this->power_switch != nullptr)
          this->power_switch->publish_state(false);
        if (this->climate != nullptr)
        {
          this->climate->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
          this->climate->publish_state();
        }
      }

      if (this->power_switch != nullptr)
      {
        switch (this->state.power)
        {
        case POWER_ON:
          this->power_switch->publish_state(true);
          break;
        default:
          this->power_switch->publish_state(false);
        }
      }
      if (this->target_temp_sensor != nullptr)
        this->target_temp_sensor->publish_state(this->state.water.set_temp);
      if (this->outlet_temp_sensor != nullptr)
        this->outlet_temp_sensor->publish_state(this->state.water.outlet_temp);
      if (this->inlet_temp_sensor != nullptr)
        this->inlet_temp_sensor->publish_state(this->state.water.inlet_temp);
    }

    void Navien::update_gas_sensors()
    {
      if (this->gas_target_temp_sensor != nullptr)
        this->gas_target_temp_sensor->publish_state(this->state.gas.set_temp);

      // Update the climate control with the current target temperature
      if (this->climate != nullptr)
      {
        //    this->climate->current_temperature = this->state.gas.outlet_temp * 9.f / 5.f + 32.f;
        // this->climate->target_temperature = this->state.gas.set_temp * 9.f / 5.f + 32.f;
        this->climate->publish_state();
      }

      if (this->gas_outlet_temp_sensor != nullptr)
        this->gas_outlet_temp_sensor->publish_state(this->state.gas.outlet_temp);

      if (this->gas_inlet_temp_sensor != nullptr)
        this->gas_inlet_temp_sensor->publish_state(this->state.gas.inlet_temp);

      if (this->gas_total_sensor != nullptr)
        this->gas_total_sensor->publish_state(this->state.gas.accumulated_gas_usage);

      if (this->sh_outlet_temp_sensor != nullptr)
        this->sh_outlet_temp_sensor->publish_state(this->state.gas.sh_outlet_temp);
      if (this->sh_return_temp_sensor != nullptr)
        this->sh_return_temp_sensor->publish_state(this->state.gas.sh_return_temp);
      if (this->gas_current_sensor != nullptr)
        this->gas_current_sensor->publish_state(this->state.gas.current_gas_usage);
      if (this->unk_sensor_g00 != nullptr)
        this->unk_sensor_g00->publish_state(this->state.gas_unknowns.unknown_00);
      if (this->unk_sensor_g01 != nullptr)
        this->unk_sensor_g01->publish_state(this->state.gas_unknowns.unknown_01);
      if (this->unk_sensor_g02 != nullptr)
        this->unk_sensor_g02->publish_state(this->state.gas_unknowns.unknown_02);
      if (this->unk_sensor_g03 != nullptr)
        this->unk_sensor_g03->publish_state(this->state.gas_unknowns.unknown_03);
      if (this->unk_sensor_g18 != nullptr)
        this->unk_sensor_g18->publish_state(this->state.gas_unknowns.unknown_18);
      if (this->unk_sensor_g19 != nullptr)
        this->unk_sensor_g19->publish_state(this->state.gas_unknowns.unknown_19);
      if (this->unk_sensor_g20 != nullptr)
        this->unk_sensor_g20->publish_state(this->state.gas_unknowns.unknown_20);
    }

    void Navien::update()
    {
      ESP_LOGV(TAG, "Conn Status: received: %d, updated: %d", this->received_cnt, this->updated_cnt);

      // here we track how many packets were received
      // since the last update
      // if Navien is connected and we receive packets, the
      // received packet count should be greater than the last time
      // we did an update. If it is not it means we no longer receive packets
      // and therefore should change our status to disconnected
      if (this->received_cnt > this->updated_cnt)
      {
        this->updated_cnt = this->received_cnt;
        this->is_connected = true;
      }
      else
      {
        // We've been disconnected. Reset counters
        this->is_connected = false;
        this->received_cnt = 0;
        this->updated_cnt = 0;
      }

      if (this->conn_status_sensor != nullptr)
        this->conn_status_sensor->publish_state(this->is_connected);

      update_water_sensors();
      update_gas_sensors();
    }

    void Navien::dump_config()
    {
      // ESP_LOGCONFIG(TAG, "Calling setup from dump_config");
      // this->setup();
    }

    void Navien::print_buffer(const uint8_t *data, size_t length)
    {
      char hex_buffer[100];
      hex_buffer[(3 * 32) + 1] = 0;
      for (size_t i = 0; i < length; i++)
      {
        snprintf(&hex_buffer[3 * (i % 32)], sizeof(hex_buffer), "%02X ", data[i]);
        if (i % 32 == 31)
        {
          ESP_LOGD(TAG, "   %s", hex_buffer);
        }
      }
      if (length % 32)
      {
        // null terminate if incomplete line
        hex_buffer[3 * (length % 32) + 2] = 0;
        ESP_LOGD(TAG, "   %s", hex_buffer);
      }
    }

#ifdef USE_SWITCH
    void NavienOnOffSwitch::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up Switch '%s'...", this->name_.c_str());

      bool initial_state = true; // this->get_initial_state_with_restore_mode().value_or(false);

      // write state before setup
      if (initial_state)
      {
        this->turn_on();
      }
      else
      {
        this->turn_off();
      }
    }

    void NavienOnOffSwitch::write_state(bool state)
    {
      if (state)
      {
        ESP_LOGD(TAG, "Turning on Navien");
        this->parent->send_turn_on_cmd();
      }
      else
      {
        ESP_LOGD(TAG, "Turning off Navien");
        this->parent->send_turn_off_cmd();
      }
      this->publish_state(state);
    }

    void NavienOnOffSwitch::set_parent(Navien *parent)
    {
      this->parent = parent;
      parent->set_power_switch(this);
    }

    void NavienOnOffSwitch::dump_config()
    {
      ESP_LOGCONFIG(TAG, "Empty custom switch");
    }
#endif

#ifdef USE_BUTTON
    void NavienHotButton::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up Button '%s'...", this->name_.c_str());
    }

    void NavienHotButton::press_action()
    {
      this->parent->send_hot_button_cmd();
    }

    void NavienHotButton::set_parent(Navien *parent)
    {
      this->parent = parent;
      //  parent->set_hot_button(this);
    }

    void NavienHotButton::dump_config()
    {
      ESP_LOGCONFIG(TAG, "Navien Hot Button");
    }
#endif

  } // namespace navien
} // namespace esphome
