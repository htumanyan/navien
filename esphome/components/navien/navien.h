#pragma once

#include <cinttypes>
#include <list>

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace navien {

typedef struct {
  /**
   * Unknown value, but could be the protocol version (very likely)
   */
  uint8_t packet_marker;
  
  /**
   * Unknown value, but could be the protocol version (very likely)
   */
  uint8_t unknown_0x05;

  /**
   * Direction of the packet. Not unlikely to be "recipient address" essentially.
   * Needs to be captured in multi-unit installation to see if it is simply a direction or a version.
   *
   * Navien to control device - 0x50, PACKET_DIRECTION_STATUS
   * Control Device to Navien - 0x0F, PACKET_DIRECTION_CONTROL
   */
  uint8_t direction;

  /**
   * There are two known packet types with somewhat overlapping
   * data but also with unique data points in each
   * 0x50 - water flow and temperature data - PACKET_TYPE_WATER
   * 0x0F - gas flow and also water temperature - PACKET_TYPE_GAS
   */
  uint8_t packet_type;

  /**
   * Unknown value. Observed to be 0x90 for navien-device direction
   * and 0x10 in device-to-navien (control packets).
   */
  uint8_t unknown_0x90;

  /**
   * Length of the packet including the header and checksum, i.e. total number of bytes
   * in the packet, including everything
   */
  uint8_t len;
} HEADER;

const uint8_t HDR_SIZE = sizeof(HEADER);
const uint8_t PACKET_MARKER = 0xF7;

const uint8_t PACKET_DIRECTION_CONTROL = 0x0F;
const uint8_t PACKET_DIRECTION_STATUS  = 0x50;
  
const uint8_t PACKET_TYPE_WATER = 0x50;
const uint8_t PACKET_TYPE_GAS   = 0x0F;

const uint16_t CHECKSUM_SEED_4B = 0x4b;
const uint16_t CHECKSUM_SEED_62 = 0x62;


const uint8_t POWER_STATUS_ON_OFF_MASK = 0x05;
  
/**                                                                                        
 * Hardcoded command packets. Some commands have no uses data. Therefore rather than assemblying a packet
 * we just pre-compute/hardcode and just send the static const buffer when need to send the command.
 *
 *                                                                                                  on/off
 *				             0xF7			 			    byte
 */								
const uint8_t TURN_OFF_CMD[]   =        {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x0b,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A};
const uint8_t TURN_ON_CMD[]    =        {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x0a,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCE};


const uint8_t HOT_BUTTON_PRESS_CMD[] =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x00,   0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A};
const uint8_t HOT_BUTTON_RELSE_CMD[] =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A};

const uint8_t RECIRC_ON_CMD[]        =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x00,   0x00, 0x00, 0x08, 0xD9, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD0};
  //  F7,05,0F,50,10,0C,4F,00,00,00,00,08,D9,00,00,00,00,00,D0

const uint8_t NAVILINK_PRESENT[]     =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x03, 0x4a, 0x00, 0x01, 0x55};
  
typedef struct {
  uint8_t unknown_06;
  uint8_t unknown_07;
  uint8_t unknown_08;
  /**
   * Kudos and credits to individuals below for this byte
   * tsquared at https://community.home-assistant.io/t/navien-esp32-navilink-interface/720567
   * David Carson (dacarson)
   * raptordemon
   *
   * This is a bitmask field
   * Observed values (binary)
   * 00100101 - turned on
   * 00100000 - turned off
   */
  uint8_t system_power; 
  uint8_t unknown_10;
  uint8_t set_temp;
  uint8_t outlet_temp;
  uint8_t inlet_temp;
  uint8_t unknown_14;
  uint8_t unknown_15;
  uint8_t unknown_17;
  //  uint8_t unknown_17;
  uint8_t water_flow;
  uint8_t unknown_19;
  uint8_t unknown_20;
  uint8_t unknown_21;
  uint8_t unknown_22;
  uint8_t unknown_23;
  /**
   * Kudos and credits to individuals below for this byte
   * tsquared at https://community.home-assistant.io/t/navien-esp32-navilink-interface/720567
   *
   * 
   */
  uint8_t system_status;
} WATER_DATA;

typedef struct {
  uint8_t  unknown_00;
  uint8_t  unknown_01;
  uint8_t  unknown_02;
  uint8_t  unknown_03;
  uint8_t  controller_version_lo;
  uint8_t  controller_version_hi;

  uint8_t  panel_version_lo;
  uint8_t  panel_version_hi;
  
  uint8_t  set_temp;
  uint8_t  outlet_temp;
  uint8_t  inlet_temp;
  uint8_t  unknown_16;
  uint8_t  unknown_17;
  uint8_t  unknown_18;
  uint8_t  unknown_19;
  uint8_t  unknown_20;
  uint8_t  current_gas_lo;
  uint8_t  current_gas_hi;
  uint8_t  cumulative_gas_lo;
  uint8_t  cumulative_gas_hi;
} GAS_DATA;

typedef struct {
  uint8_t unknown_00;   // always 4F?
  uint8_t unknown_01;   // 00
  uint8_t unknown_02;   // 00
  uint8_t unknown_temp; // 5E - 117F
  uint8_t unknown_04;   // 00
  uint8_t unknown_05;   // 00
  uint8_t unknown_06;   // 00
  uint8_t unknown_07;   // 00
  uint8_t unknown_08;   // 00
  uint8_t unknown_09;   // 00
  uint8_t unknown_10;   // 00
  uint8_t unknown_11;   // 00 
} CMD_DATA;
  
typedef union{
    struct{
      HEADER  hdr;
      union{
	GAS_DATA   gas;
	WATER_DATA water;
      };
    };
    uint8_t    raw_data[128];
} RECV_BUFFER;


typedef enum _DEVICE_POWER_STATE{
  POWER_OFF,
  POWER_ON
} DEVICE_POWER_STATE;
  
typedef struct{
  uint16_t           controller_version;
  uint16_t           panel_version;
  DEVICE_POWER_STATE power;
} NAVIEN_STATE;
  
typedef enum{
  INITIAL,
  MARKER_FOUND,
  HEADER_PARSED
} READ_STATE;

typedef struct _NAVIEN_CMD{
  const uint8_t * buffer;
  uint8_t   len;
  _NAVIEN_CMD(const uint8_t * b, uint8_t l): buffer(b), len(l) {}
} NAVIEN_CMD;
  
class Navien : public PollingComponent, public uart::UARTDevice {
public:
  virtual float get_setup_priority() const { return setup_priority::HARDWARE; }
  virtual void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;

protected:
  // Debug helper to print hex buffers
  static void print_buffer(const uint8_t *data, size_t length);


  static uint8_t t2f(uint8_t);
  static float flow2gpm(uint8_t f);
  static uint8_t t2c(uint8_t);
  static float flow2lpm(uint8_t f);
  static float usage2cum(uint16_t f);

  /**
   * 
   */
  static uint8_t checksum(const uint8_t * buffer, uint8_t len, uint16_t seed);
  
protected:
  //  void    clear_buffer();
  bool    seek_to_marker();


  /**
   * Data extraction routines.
   * Copy the raw data from recv_buffer to internal representation
   * in this->state where it is stored and reported upon "update" calls.
   */

  // Common entry point that is always called upon receipt of a valid packet
  // calls parse_water/gas depending on the paket type
  void parse_packet();

  // Called when we observe a control packet from another
  // control device (not us). 
  void parse_control_packet();

  // Called when we receive a status packet from Navien device 
  void parse_status_packet();
  
  // Called then the direction is from Navien to reporting device
  // and type field is PACKET_TYPE_WATER 
  void parse_water();

  // Called then the direction is from Navien to reporting device
  // and the type field is PACKET_TYPE_GAS 
  void parse_gas();


public:
  /**
   * Send commands
   */
  void send_turn_on_cmd();
  void send_turn_off_cmd();
  void send_hot_button_cmd();
  
protected:
  void send_cmd(const uint8_t * buffer, uint8_t len);
  
protected:
  // Keeps track of the state machine and iterates through
  // initialized -> marker found -> header parsed -> data parsed -> initialized
  READ_STATE   recv_state;

  // Data received off the wire
  RECV_BUFFER  recv_buffer;
  //uint8_t      recv_ptr;
  //bool         found_marker;

  // Data, extracted from gas and water packers and stored
  // Once the "update" is called this data gets reported to readers.
  NAVIEN_STATE state;

  // How many packets were received
  uint32_t received_cnt;

  // How many packets were updated
  uint32_t updated_cnt;
  
  // true if connected to Navien.
  // otherwie - false.
  bool is_connected;

  std::list<NAVIEN_CMD> cmd_buffer;
public:
  void set_target_temp_sensor(sensor::Sensor *sensor) { target_temp_sensor = sensor; }
  void set_inlet_temp_sensor(sensor::Sensor *sensor) { inlet_temp_sensor = sensor; }
  void set_outlet_temp_sensor(sensor::Sensor *sensor) { outlet_temp_sensor = sensor; }
  void set_water_flow_sensor(sensor::Sensor *sensor) { water_flow_sensor = sensor; }
  void set_current_gas_sensor(sensor::Sensor *sensor) { current_gas_sensor = sensor; }
  void set_accumulated_gas_sensor(sensor::Sensor *sensor) { accumulated_gas_sensor = sensor; }

#ifdef USE_SWITCH
  void set_power_switch(switch_::Switch * ps){power_switch = ps;}
#endif
protected:
  /**
   * Sensor definitions
   */
  sensor::Sensor *target_temp_sensor = nullptr;
  sensor::Sensor *outlet_temp_sensor = nullptr;
  sensor::Sensor *inlet_temp_sensor = nullptr;
  sensor::Sensor *water_flow_sensor = nullptr;
  sensor::Sensor *current_gas_sensor = nullptr;
  sensor::Sensor *accumulated_gas_sensor = nullptr;

#ifdef USE_SWITCH
  switch_::Switch *power_switch;
#endif
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
