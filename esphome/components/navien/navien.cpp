#include <cmath>
#include <string>

#include "esphome.h"
#include "esphome/core/log.h"
#include "navien.h"

namespace esphome {
namespace navien {

  static const char *TAG = "navien.sensor";

namespace {
class NavienEspUartAdapter : public NavienUartI {
 public:
  void set_uart(esphome::uart::UARTComponent *uart) { uart_ = uart; }
  int available() override { return uart_ ? uart_->available() : 0; }
  uint8_t peek_byte(uint8_t *byte) override {
    if (!uart_) return 0;
    return uart_->peek_byte(byte) ? 1 : 0;
  }
  uint8_t read_byte(uint8_t *byte) override {
    if (!uart_) return 0;
    return uart_->read_byte(byte) ? 1 : 0;
  }
  bool read_array(uint8_t *data, uint8_t len) override {
    if (!uart_) return false;
    return uart_->read_array(data, len);
  }
  void write_array(const uint8_t *data, uint8_t len) override {
    if (!uart_) return;
    uart_->write_array(data, len);
  }

 private:
  esphome::uart::UARTComponent *uart_ = nullptr;
};

NavienEspUartAdapter global_uart_adapter;
}  // namespace

NavienBase::NavienBase() : navien_link_(nullptr), uart_(nullptr), src_(0), is_rt(false) {}

void NavienBase::setup() {
  if (!uart_) {
    ESP_LOGE(TAG, "UART interface not set");
    return;
  }
  global_uart_adapter.set_uart(uart_);
  navien_link_ = NavienLink::get_instance(&global_uart_adapter);
  if (navien_link_ != nullptr) {
    navien_link_->add_visitor(this, src_);
  } else {
    ESP_LOGE(TAG, "Failed to acquire NavienLink singleton");
  }
}

  // NavienBase implementation
// NavienLinkEsp removed. set_link now uses NavienLink.

void NavienBase::send_turn_on_cmd() {
  if (navien_link_) navien_link_->send_turn_on_cmd();
}
void NavienBase::send_turn_off_cmd() {
  if (navien_link_) navien_link_->send_turn_off_cmd();
}
void NavienBase::send_hot_button_cmd() {
  if (navien_link_) navien_link_->send_hot_button_cmd();
}
void NavienBase::send_dhw_set_temp_cmd(float temp) {
  if (navien_link_) navien_link_->send_dhw_set_temp_cmd(temp);
}
void NavienBase::send_scheduled_recirculation_on_cmd() {
  if (navien_link_) navien_link_->send_scheduled_recirculation_on_cmd();
}
void NavienBase::send_scheduled_recirculation_off_cmd() {
  if (navien_link_) navien_link_->send_scheduled_recirculation_off_cmd();
}

  // Navien implementation
  void Navien::setup() {
    NavienBase::setup();
    this->state.power = POWER_OFF;
  }

  void Navien::on_water(const WATER_DATA & water, uint8_t src){
    ESP_LOGD(TAG, "SRC:0x%02X Received Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, Flow: 0x%02X, Sys Power: 0x%02X, Sys Status: 0x%02X, Recirc Enabled: 0x%02X",
             src,
             water.dhw_set_temp,
             water.inlet_temp,
             water.outlet_temp,
             water.water_flow,
             water.system_power,
             water.system_status,
             water.recirculation_enabled);

    if (water.system_power & POWER_STATUS_ON_OFF_MASK){
      state.power = POWER_ON;
    }else{
      state.power = POWER_OFF;
    }

    if (this->state.device_type == NCB_H) {
      // NCB_H units don't seem to report their recirculation setting in the packets
      /* TODO this might also be true of other device types that use packet type 0x06, 
         see comment by recirc_running below */
      state.recirculation = RECIRC_UNKNOWN;
    }else if (water.system_status & SYS_STATUS_FLAG_RECIRC_INT_SCHEDULED){
      state.recirculation = RECIRC_INT_SCHEDULED;
    }else if (water.system_status & SYS_STATUS_FLAG_RECIRC_EXT_SCHEDULED){
      state.recirculation = RECIRC_EXT_SCHEDULED;
    }else if (state.hotbutton_mode_enabled){
      // hotbutton_mode_enabled actually comes from gas packets
      state.recirculation = RECIRC_EXT_HOTBUTTON;
    }else{
      state.recirculation = RECIRC_OFF;
    }
    
    state.heating_mode = static_cast<DEVICE_HEATING_MODE>(water.heating_mode);

    state.operating_state = static_cast<OPERATING_STATE>(water.operating_state);
    // Update the counter that will be used in assessment
    // of whether we're connected to navien or not
    this->received_cnt++;
    this->state.water.boiler_active = water.boiler_active & 0x01;
    this->state.water.dhw_set_temp = NavienLink::t2c(water.dhw_set_temp);
    this->state.water.outlet_temp = NavienLink::t2c(water.outlet_temp);
    this->state.water.inlet_temp = NavienLink::t2c(water.inlet_temp);
    this->state.water.flow_lpm = NavienLink::flow2lpm(water.water_flow);
    this->state.water.utilization = water.operating_capacity * 0.5f;
    // Recirculation running detection varies by device type
    /* TODO detecting this by the second byte of the header might be better, since it's
       0x05 on NPE and 0x06 on NCB_H, and who knows what on other device types, but for now
       this is a less invasive code change since the header isn't available here yet */
    if (this->state.device_type == NCB_H) {
        // NCB_H units: pump running indicated by system_power bit 5
        this->state.water.recirc_running = water.system_power & RECIRCULATION_ON_OFF_MASK;
    } else {
        // NPE and other units: scheduled (heating_mode 0x08) or hotbutton (byte 33 bit 0)
        this->state.water.recirc_running =
            (water.heating_mode & HEATING_MODE_DOMESTIC_HOT_WATER_RECIRCULATING) ||
            (water.recirculation_enabled & RECIRC_STATUS_FLAG_HOTBUTTON_ON);
    }
    this->state.water.scheduled_recirc_allowed = water.recirculation_enabled & RECIRC_STATUS_FLAG_SCHEDULED_ON;

    if (this->is_rt)
      this->update_water_sensors();
  }

  void Navien::on_gas(const GAS_DATA & gas, uint8_t src){
    ESP_LOGD(TAG, "SRC:0x%02X Received Gas DHW Temp: 0x%02X, Inlet: 0x%02X, Outlet: 0x%02X, SH Temp: 0x%02X",
       src,
       gas.dhw_set_temp,
       gas.inlet_temp,
       gas.outlet_temp,
       gas.sh_set_temp
    );

    ESP_LOGD(TAG, "SRC:0x%02X Received Accumulated: 0x%02X 0x%02X, Current Gas: 0x%02X 0x%02X, Capacity Util: 0x%02X",
       src,
       gas.cumulative_gas_hi,
       gas.cumulative_gas_lo,
       gas.current_gas_hi,
       gas.current_gas_lo,
       gas.heat_capacity
    );

    // Update the counter that will be used in assessment
    // of whether we're connected to navien or not
    this->received_cnt++;

    this->state.gas.dhw_set_temp = NavienLink::t2c(gas.dhw_set_temp);
    this->state.gas.outlet_temp = NavienLink::t2c(gas.outlet_temp);
    this->state.gas.inlet_temp = NavienLink::t2c(gas.inlet_temp);
    this->state.gas.sh_set_temp = NavienLink::t2c(gas.sh_set_temp);
    this->state.gas.sh_outlet_temp = NavienLink::t2c(gas.sh_outlet_temp);
    this->state.gas.sh_return_temp = NavienLink::t2c(gas.sh_return_temp);
    this->state.device_type = static_cast<DEVICE_TYPE>(gas.device_type);
    this->state.gas.heat_capacity = gas.heat_capacity * 0.5f;
    this->state.gas.total_dhw_usage = gas.cumulative_domestic_usage_cnt_hi << 8 | gas.cumulative_domestic_usage_cnt_lo;
    this->state.gas.total_operating_time = gas.total_operating_time_hi << 8 | gas.total_operating_time_lo;
    this->state.gas.accumulated_gas_usage = gas.cumulative_gas_hi << 8 | gas.cumulative_gas_lo;
    this->state.gas.current_gas_usage = gas.current_gas_hi << 8 | gas.current_gas_lo;
    this->state.gas.cumulative_dwh_usage_hours = gas.cumulative_dwh_usage_hours_hi << 8 | gas.cumulative_dwh_usage_hours_lo;
    this->state.gas.cumulative_sh_usage_hours = gas.cumulative_sh_usage_hours_hi << 8 | gas.cumulative_sh_usage_hours_lo;
    
    if (gas.system_status_2 & SYS_STATUS_2_DISPLAY_UNITS){
      this->state.units = FARENHEIT;
    }else{
      this->state.units = CELSIUS;
    }

    std::string contVers = std::to_string(gas.controller_version);
    if (contVers.length() < 2){
      contVers.insert(0, 2 - contVers.length(), '0');
    }
    this->state.controller_version = contVers.substr(0, 1) + "." + contVers.substr(1, 1);

    std::string panVers = std::to_string(gas.panel_version);
    if (panVers.length() < 2){
      panVers.insert(0, 2 - panVers.length(), '0');
    }
    this->state.panel_version = panVers.substr(0, 1) + "." + panVers.substr(1, 1);

    this->state.days_since_install = gas.days_since_install_hi << 8 | gas.days_since_install_lo;
    this->state.cumulative_domestic_usage_cnt = gas.cumulative_domestic_usage_cnt_hi << 8 | gas.cumulative_domestic_usage_cnt_lo;
    this->state.hotbutton_mode_enabled = gas.system_status_2 & SYS_STATUS_2_HOTBUTTON_ENABLED;

    if (this->is_rt)
      this->update_gas_sensors();
  }

  void Navien::on_error(){
    ESP_LOGW(TAG, "Communications interrupted, resetting states!");

    sensor::Sensor *sensors[] = {
      this->dhw_set_temp_sensor,
      this->outlet_temp_sensor,
      this->inlet_temp_sensor,
      this->gas_dhw_set_temp_sensor,
      this->gas_outlet_temp_sensor,
      this->gas_inlet_temp_sensor,
      this->water_flow_sensor,
      this->water_utilization_sensor,
      this->gas_total_sensor,
      this->gas_current_sensor,
      this->sh_set_temp_sensor,
      this->sh_outlet_temp_sensor,
      this->sh_return_temp_sensor,
      this->heat_capacity_sensor,
      this->total_dhw_usage_sensor,
      this->total_operating_time_sensor,
      this->cumulative_dwh_usage_hours_sensor,
      this->cumulative_sh_usage_hours_sensor,
      this->cumulative_domestic_usage_cnt_sensor,
      this->days_since_install_sensor
    };

    for (sensor::Sensor *s : sensors) {
        if (s != nullptr){
            s->publish_state(NAN);
        }
    }

    text_sensor::TextSensor *text_sensors[] = {
      this->controller_version_sensor,
      this->panel_version_sensor,
      this->heating_mode_sensor,
      this->device_type_sensor,
      this->operating_state_sensor,
      this->recirc_mode_sensor
    };

    for (text_sensor::TextSensor *t : text_sensors) {
        if (t != nullptr){
            t->publish_state("");
        }
    }
    
    binary_sensor::BinarySensor *binary_sensors[] = {
      this->boiler_active_sensor,
      this->conn_status_sensor,
      this->recirc_running_sensor,
      this->other_navilink_installed_sensor
    };

    for (binary_sensor::BinarySensor *b : binary_sensors) {
        if (b != nullptr){
            b->publish_state(false);
        }
    }

    this->is_connected = false;
  }

  void Navien::update_water_sensors(){
    if (this->water_flow_sensor != nullptr)
      this->water_flow_sensor->publish_state(this->state.water.flow_lpm);

    if (this->water_utilization_sensor != nullptr)
      this->water_utilization_sensor->publish_state(this->state.water.utilization);

    if (this->heating_mode_sensor != nullptr){
      std::string heating_mode_str = heat_mode_to_str(this->state.heating_mode);
      this->heating_mode_sensor->publish_state(heating_mode_str);
    }
    if (this->boiler_active_sensor != nullptr){
      this->boiler_active_sensor->publish_state(this->state.water.boiler_active);
    }
    if (this->recirc_running_sensor != nullptr){
      this->recirc_running_sensor->publish_state(this->state.water.recirc_running);
    }

#ifdef USE_CLIMATE
    // Update the climate control with the current target temperature
    if (this->climate != nullptr){
      switch(this->state.power){
      case POWER_ON:
        this->climate->mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
        break;
      default:
        this->climate->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
      }

      this->climate->current_temperature = this->state.water.outlet_temp;
      this->climate->target_temperature = this->state.water.dhw_set_temp;
      this->climate->publish_state();
    }
#endif

    if (this->recirc_mode_sensor != nullptr) {
      this->recirc_mode_sensor->publish_state(device_recirc_mode_to_str(this->state.recirculation));
    }

#ifdef USE_SWITCH
    if (this->power_switch != nullptr){
      switch(this->state.power){
      case POWER_ON:
        this->power_switch->publish_state(true);
        break;
      default:
        this->power_switch->publish_state(false);
      }
    }

    if (this->allow_recirc_switch != nullptr){
      this->allow_recirc_switch->publish_state(this->state.water.scheduled_recirc_allowed);
    }
#endif

    if (this->dhw_set_temp_sensor != nullptr)
      this->dhw_set_temp_sensor->publish_state(this->state.water.dhw_set_temp);
    if (this->outlet_temp_sensor != nullptr)
      this->outlet_temp_sensor->publish_state(this->state.water.outlet_temp);
    if (this->inlet_temp_sensor != nullptr)
      this->inlet_temp_sensor->publish_state(this->state.water.inlet_temp);
    if (this->operating_state_sensor != nullptr){
      std::string operating_state_str = op_state_to_str(this->state.operating_state);
      this->operating_state_sensor->publish_state(operating_state_str);
    }
  }

  void Navien::update_gas_sensors(){
    if (this->dhw_set_temp_sensor != nullptr)
      this->dhw_set_temp_sensor->publish_state(this->state.gas.dhw_set_temp);

#ifdef USE_CLIMATE
    // Update the climate control with the current target temperature
    if (this->climate != nullptr){
      this->climate->publish_state();
    }
#endif
  
  if (this->outlet_temp_sensor != nullptr)
    this->outlet_temp_sensor->publish_state(this->state.gas.outlet_temp);

  if (this->inlet_temp_sensor != nullptr)
      this->inlet_temp_sensor->publish_state(this->state.gas.inlet_temp);

    if (this->gas_total_sensor != nullptr)
      this->gas_total_sensor->publish_state(this->state.gas.accumulated_gas_usage);

    if (this->device_type_sensor != nullptr){
      std::string device_type_str = device_type_to_str(this->state.device_type);
      this->device_type_sensor->publish_state(device_type_str);
    }
    if (this->heat_capacity_sensor != nullptr)
      this->heat_capacity_sensor->publish_state(this->state.gas.heat_capacity);
    if (this->sh_set_temp_sensor != nullptr)
      this->sh_set_temp_sensor->publish_state(this->state.gas.sh_set_temp);
    if (this->sh_outlet_temp_sensor != nullptr)
      this->sh_outlet_temp_sensor->publish_state(this->state.gas.sh_outlet_temp);
    if (this->sh_return_temp_sensor != nullptr)
      this->sh_return_temp_sensor->publish_state(this->state.gas.sh_return_temp);
    if (this->gas_current_sensor != nullptr)
      this->gas_current_sensor->publish_state(this->state.gas.current_gas_usage);
    if (this->total_dhw_usage_sensor != nullptr)
      this->total_dhw_usage_sensor->publish_state(this->state.gas.total_dhw_usage);
    if (this->total_operating_time_sensor != nullptr)
      this->total_operating_time_sensor->publish_state(this->state.gas.total_operating_time);
    if (this->cumulative_dwh_usage_hours_sensor != nullptr)
      this->cumulative_dwh_usage_hours_sensor->publish_state(this->state.gas.cumulative_dwh_usage_hours);
    if (this->cumulative_sh_usage_hours_sensor != nullptr)
      this->cumulative_sh_usage_hours_sensor->publish_state(this->state.gas.cumulative_sh_usage_hours);
    if (this->cumulative_domestic_usage_cnt_sensor != nullptr)
      this->cumulative_domestic_usage_cnt_sensor->publish_state(this->state.cumulative_domestic_usage_cnt);
    if (this->days_since_install_sensor != nullptr)
      this->days_since_install_sensor->publish_state(this->state.days_since_install);
    if (this->controller_version_sensor != nullptr)
      this->controller_version_sensor->publish_state(this->state.controller_version);
    if (this->panel_version_sensor != nullptr)
      this->panel_version_sensor->publish_state(this->state.panel_version);
  }

  void Navien::loop() {
    if (navien_link_ && src_ == 0) {
      navien_link_->receive();
    }
  }

  void Navien::update() {
    ESP_LOGV(TAG, "Conn Status: received: %d, updated: %d", this->received_cnt, this->updated_cnt);

    // here we track how many packets were received
    // since the last update
    // if Navien is connected and we receive packets, the
    // received packet count should be greater than the last time
    // we did an update. If it is not it means we no longer receive packets
    // and therefore should change our status to disconnected
    if (this->received_cnt > this->updated_cnt){
      this->updated_cnt = this->received_cnt;
      this->is_connected = true;
    }else{
      // We've been disconnected. Reset counters
      this->is_connected = false;
      this->received_cnt = 0;
      this->updated_cnt = 0;
    }

    if (this->conn_status_sensor != nullptr)
      this->conn_status_sensor->publish_state(this->is_connected);

    if (this->other_navilink_installed_sensor != nullptr)
      this->other_navilink_installed_sensor->publish_state(this->navien_link_->is_other_navilink_installed());

    update_water_sensors();
    update_gas_sensors();
  }

  
  void Navien::dump_config(){
    //ESP_LOGCONFIG(TAG, "Calling setup from dump_config");
    //this->setup();
  }

  std::string Navien::op_state_to_str(OPERATING_STATE state) {
    switch(state){
      case STANDBY:
        return "Standby";
      case STARTUP:
        return "Startup";
      case DEMAND:
        return "Demand";
      case PRE_PURGE_1:
        return "Pre Purge Stage 1";
      case PRE_PURGE_2:
        return "Pre Purge Stage 2";
      case PRE_IGNITION:
        return "Pre-Ignition";
      case IGNITION:
        return "Ignition";
      case FLAME_ON:
        return "Flame On";
      case RAMP_UP:
        return "Ramp Up";
      case ACTIVE_COMBUSTION:
        return "Active Combustion";
      case WATER_ADJUSTMENT_VALVE_OPERATION:
        return "Water Adjustment Valve Operation";
      case FLAME_OFF:
        return "Flame Off";
      case POST_PURGE_1:
        return "Post Purge Stage 1";
      case POST_PURGE_2:
        return "Post Purge Stage 2";
      case SHUTTING_DOWN:
        return "Shutting Down";
      case DHW_WAIT:
        return "DHW Wait/Set Point Match";
      default:
        char buf[64];
        std::snprintf(
            buf,
            sizeof(buf),
            "Unknown (%u)",
            static_cast<unsigned int>(state));

        return buf;
      }
  }

  std::string Navien::heat_mode_to_str(DEVICE_HEATING_MODE mode) {
    switch(mode){
      case HEATING_MODE_IDLE:
        return "Idle";
      case HEATING_MODE_SPACE_HEATING:
        return "Space Heating";
      case HEATING_MODE_DOMESTIC_HOT_WATER_DEMAND:
        return "Domestic Hot Water";
      case HEATING_MODE_DOMESTIC_HOT_WATER_RECIRCULATING:
        return "DHW Recirculating";
      default:
        return "unknown";
    }
  }

  std::string Navien::device_type_to_str(DEVICE_TYPE type) {
    switch(type){
      case NO_DEVICE:
        return "No Device";
      case NPE:
        return "NPE";
      case NCB:
        return "NCB";
      case NHB:
        return "NHB";
      case CAS_NPE:
        return "CAS NPE";
      case CAS_NHB:
        return "CAS NHB";
      case NFB:
        return "NFB";
      case CAS_NFB:
        return "CAS NFB";
      case NFC:
        return "NFC";
      case NPN:
        return "NPN";
      case CAS_NPN:
        return "CAS NPN";
      case NPE2:
        return "NPE2";
      case CAS_NPE2:
        return "CAS NPE2";
      case NCB_H:
        return "NCB-H";
      case NVW:
        return "NVW";
      case CAS_NVW:
        return "CAS NVW";
      default:
        return "unknown";
    }
  }

  std::string Navien::device_recirc_mode_to_str(DEVICE_RECIRC_MODE state) {
    switch(state){
      case RECIRC_OFF:
        return "Off";
      case RECIRC_EXT_HOTBUTTON:
        return "External HotButton";
      case RECIRC_EXT_SCHEDULED:
        return "External Scheduled";
      case RECIRC_INT_SCHEDULED:
        return "Internal Scheduled";
      case RECIRC_UNKNOWN:
      default:
        return "unknown";
    }
  }

  void Navien::print_buffer(const uint8_t *data, size_t length) {
    char hex_buffer[100];
    hex_buffer[(3 * 32) + 1] = 0;
    for (size_t i = 0; i < length; i++) {
      snprintf(&hex_buffer[3 * (i % 32)], sizeof(hex_buffer), "%02X ", data[i]);
      if (i % 32 == 31) {
        ESP_LOGD(TAG, "   %s", hex_buffer);
      }
    }
    if (length % 32) {
      // null terminate if incomplete line
      hex_buffer[3 * (length % 32) + 2] = 0;
      ESP_LOGD(TAG, "   %s", hex_buffer);
    }
  }


#ifdef USE_SWITCH
void NavienOnOffSwitch::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Switch '%s'...", this->name_.c_str());

  bool initial_state = true;//this->get_initial_state_with_restore_mode().value_or(false);

  // write state before setup
  if (initial_state) {
    this->turn_on();
  } else {
    this->turn_off();
  }

}

void NavienOnOffSwitch::write_state(bool state) {
  if (state){
    ESP_LOGD(TAG, "Turning on Navien");
    this->parent->send_turn_on_cmd();
  }else{
    ESP_LOGD(TAG, "Turning off Navien");
    this->parent->send_turn_off_cmd();
  }
  this->publish_state(state);
}

void NavienOnOffSwitch::set_parent(Navien * parent) {
  this->parent = parent;
  parent->set_power_switch(this);
}

void NavienOnOffSwitch::dump_config(){
  ESP_LOGCONFIG(TAG, "Empty custom switch");
}

void NavienAllowScheduledRecircSwitch::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Allow Recirculation Switch '%s'...", this->name_.c_str());

  bool initial_state = false;

  // write state before setup
  if (initial_state) {
    this->turn_on();
  } else {
    this->turn_off();
  }
}

void NavienAllowScheduledRecircSwitch::write_state(bool state) {
  if (state){
    ESP_LOGD(TAG, "Turning on Scheduled Recirculation");
    this->parent->send_scheduled_recirculation_on_cmd();
  }else{
    ESP_LOGD(TAG, "Turning off Scheduled Recirculation");
    this->parent->send_scheduled_recirculation_off_cmd();
  }
  this->publish_state(state);
}

void NavienAllowScheduledRecircSwitch::set_parent(Navien * parent) {
  this->parent = parent;
  parent->set_allow_recirc_switch(this);
}

void NavienAllowScheduledRecircSwitch::dump_config(){
    ESP_LOGCONFIG(TAG, "Navien Allow Scheduled Recirculation Switch");
}
#endif


#ifdef USE_BUTTON
void NavienHotButton::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Button '%s'...", this->name_.c_str());
}

void NavienHotButton::press_action(){
  this->parent->send_hot_button_cmd();
}

void NavienHotButton::set_parent(Navien * parent) {
  this->parent = parent;
  //  parent->set_hot_button(this);
}

void NavienHotButton::dump_config(){
  ESP_LOGCONFIG(TAG, "Navien Hot Button");
}
#endif

}  // namespace navien
}  // namespace esphome
