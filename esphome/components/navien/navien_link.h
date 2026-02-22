#pragma once

#include <cinttypes>
#include <list>

#include "esphome/core/helpers.h"


#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#include "esphome/components/uart/uart.h"

#include "navien_proto.h"

namespace esphome {
namespace navien {

  
typedef enum{
  INITIAL,
  MARKER_FOUND,
  HEADER_PARSED
} READ_STATE;

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
  
typedef struct _NAVIEN_CMD{
  uint8_t   buffer[64];
  uint8_t   len;
  _NAVIEN_CMD(const uint8_t * b, uint8_t l) {
    len = std::min(l, static_cast<uint8_t>(sizeof(buffer)));
    memcpy(buffer, b, len);
  }
} NAVIEN_CMD;


 /**
  * UART connectivity interface that abstracts away the details of UART implementation
  * which makes it possible to use the navien parser across platforms (esphome, aruin etc.)
  */
class NavienUartI{
public:
  virtual int     available() = 0;
  virtual uint8_t peek_byte(uint8_t * byte) = 0; 
  virtual uint8_t read_byte(uint8_t * byte) = 0;
  virtual bool read_array(uint8_t * data, uint8_t len) = 0;
  virtual void write_array(const uint8_t * data, uint8_t len) = 0; 
};

/**
 * Callback interface (per Gof Visitor pattern) for NavienLink to call
 * when it receives and parses various packets (water, gas, etc).
 */
class NavienLinkVisitorI{
public:
  /**
   * Called then the direction is from Navien to reporting device
   * and type field is PACKET_DST_WATER
   * @param water - the data payload of the water packet
   * @param src - the source address from the packet header
   */  
  virtual void on_water(const WATER_DATA & water, uint8_t src) = 0;

  /**
   * Called then the direction is from Navien to reporting device
   * and type field is PACKET_DST_GAS
   * @param gas - the data payload of the gas packet
   * @param src - the source address from the packet header
   */  
  virtual void on_gas(const GAS_DATA & gas, uint8_t src)   = 0;
  virtual void on_error()     = 0;
};


/**
 * Encapsulates the knowledge of Navien protocol, relies on generalized Uart representation
 * for connectivity and calls the methods of NavienLinkVisitor callback when receives respective packets.
 */
class NavienLink  {
public:
  static NavienLink* get_instance(NavienUartI* uart = nullptr);
public:
  static const uint8_t NAVIEN_CASCADE_MAX = 16;
  NavienLink(NavienUartI* u) : uart(u) {
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
   * Reads whaterver data came through UART and attempts to interpret it as Navien protocol data.
   * In case of success it calls methods of NavienLinkVisitorI with gas or water data (or an errror).
   * It also send the regular heartbeat packets to the Navien heater
   * Lastly, depending on the outcome of packet receiption this method updates the state of Navilink - connected or not
   */
  void receive(); // Implementation should call all registered visitors

  /**
   * Returns true if connected, otherwise false.
   *
  bool is_connected(){return this->connected;}
  */
  
  /**
   * Returns true if we're sharing the RS485 bus with another NaviLink-like device, otherwise false.
   */
  bool is_other_navilink_installed(){return this->other_navilink_installed;}
  
  /**
   * Send commands
   */
  void send_turn_on_cmd();
  void send_turn_off_cmd();
  void send_hot_button_cmd();
  void send_scheduled_recirculation_on_cmd();
  void send_scheduled_recirculation_off_cmd();
  void send_dhw_set_temp_cmd(float temp);

  
public:
  // Debug helper to print hex buffers
  static void print_buffer(const uint8_t *data, size_t length);

  static uint8_t t2f(uint8_t);
  static float flow2gpm(uint8_t f);
  static float t2c(uint8_t);
  static float flow2lpm(uint8_t f);
  static float ot2c(uint8_t);

protected:
  /**
   * Calculate the packet checksum
   * @param buffer - the input buffer for the checksum to be calculated on
   * @param len    - buffer length
   * @param seed   - the seed value. There are two known values, either CHECKSUM_SEED_4B or CHECKSUM_SEED_62
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

protected:
  /**
   * Send command to Navien unit.
   *
   * @param buffer - command to be sent.
   * @param len - the length of buffer
   * @param tries - number of times to send the command
   */
  void send_cmd(const uint8_t * buffer, uint8_t len, uint8_t tries = 2);
  void on_error();
  
protected:
  // Uart Send/Receive facility
  NavienUartI*       uart;

  // Callback to be called when various packet types
  // are received
  // Visitor array for callbacks
  NavienLinkVisitorI *visitors_[NAVIEN_CASCADE_MAX];

  // Keeps track of the state machine and iterates through
  // initialized -> marker found -> header parsed -> data parsed -> initialized
  READ_STATE   recv_state = INITIAL;

  // Data received off the wire
  RECV_BUFFER  recv_buffer;

  // Flag indicating if we've seen control packets that we didn't send, which means an actual NaviLink is also present
  bool other_navilink_installed = false;

  // Buffer for queued commands.
  // TODO: add thread safety - cmd_buffer is used in different thread contexts
  std::list<NAVIEN_CMD> cmd_buffer;
};

  
}  // namespace navien
}  // namespace esphome
