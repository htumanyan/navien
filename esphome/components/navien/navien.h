#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
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

/**                                                                                        
 * Hardcoded command packets. Some commands have no used data. Therefore rather than assemblying a packet
 * we just pre-compute/hardcode and just send the static const buffer when need to send the command.
 *
 *                                                                                        on/off
 *				  0xF7							   byte
 */								
const uint8_t TURN_OFF_CMD[] = {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x0b,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A};
const uint8_t TURN_ON_CMD[] =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x0a,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCE};
  
typedef struct {
  uint8_t unknown_05;
  uint8_t unknown_06;
  uint8_t unknown_07;
  uint8_t unknown_08;
  uint8_t unknown_09;
  uint8_t set_temp;
  uint8_t outlet_temp;
  uint8_t inlet_temp;
  uint8_t unknown_11;
  uint8_t unknown_12;
  uint8_t unknown_13;
  uint8_t unknown_14;
  uint8_t water_flow;
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

  
typedef struct{
  struct{
    uint8_t set_temp;
    uint8_t outlet_temp;
    uint8_t inlet_temp;
    float flow_lpm;
  } water;
  struct{
    uint8_t  set_temp;
    uint8_t  outlet_temp;
    uint8_t  inlet_temp;
    uint16_t controller_version;
    uint16_t panel_version;
    uint16_t accumulated_gas_usage;
    uint16_t current_gas_usage;
  } gas;
} NAVIEN_STATE;
  
typedef enum{
  INITIAL,
  MARKER_FOUND,
  HEADER_PARSED
} READ_STATE;


  
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
  
protected:
  void send_cmd(const uint8_t * buffer, uint8 len);
  
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

public:
  void set_target_temp_sensor(sensor::Sensor *sensor) { target_temp_sensor = sensor; }
  void set_inlet_temp_sensor(sensor::Sensor *sensor) { inlet_temp_sensor = sensor; }
  void set_outlet_temp_sensor(sensor::Sensor *sensor) { outlet_temp_sensor = sensor; }
  void set_water_flow_sensor(sensor::Sensor *sensor) { water_flow_sensor = sensor; }


protected:
  /**
   * Sensor definitions
   */
  sensor::Sensor *target_temp_sensor;
  sensor::Sensor *outlet_temp_sensor;
  sensor::Sensor *inlet_temp_sensor;
  sensor::Sensor *water_flow_sensor;
};

class NavienOnOffSwitch : public switch_::Switch, public Component {
    protected:
      Navien * parent;
    public:
      void setup() override;

      void set_parent(Navien * parent) {this->parent = parent;}
      void write_state(bool state) override;
      void dump_config() override;
};
    
}  // namespace navien
}  // namespace esphome
