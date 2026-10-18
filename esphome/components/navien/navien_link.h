#pragma once

#include <algorithm>
#include <cinttypes>
#include <cstring>
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
   * System type byte (header byte 1) from the unit's status packets,
   * SYS_TYPE_UNKNOWN until the first valid status packet.
   */
  uint8_t get_unit_sys_type() const { return this->unit_sys_type_; }

  /**
   * Force the system type byte used in our control packets.
   * 0 (default) picks it automatically from the unit's status packets,
   * see command_sys_type().
   */
  void set_command_sys_type(uint8_t sys_type) { this->command_sys_type_override_ = sys_type; }

  /**
   * System type byte our control packets will carry.
   */
  uint8_t command_sys_type() const;

  /**
   * Raw capture: every frame received or sent, and any bytes found between
   * frames, is logged as one hex line under the "navien.raw" tag. On a DEBUG
   * build that always happens (at DEBUG level). Enabling capture logs the
   * same lines at INFO, so it also works on an INFO build.
   */
  void set_raw_capture(bool enable) { this->raw_capture_ = enable; }
  bool is_raw_capture() const { return this->raw_capture_; }

  /**
   * Send commands
   */
  void send_turn_on_cmd();
  void send_turn_off_cmd();
  void send_hot_button_cmd();
  void send_scheduled_recirculation_on_cmd();
  void send_scheduled_recirculation_off_cmd();
  void send_dhw_set_temp_cmd(float temp);
  // Space-heating (supply water) setpoint on combi / boiler units, deg C.
  void send_sh_set_temp_cmd(float temp);

  
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

  // Largest body (data + checksum) that fits in recv_buffer after the header.
  static constexpr uint8_t MAX_BODY_LEN = sizeof(RECV_BUFFER) - HDR_SIZE - 1;

  // System type byte reported by the unit, and an optional user override for
  // the one we send (0 = automatic).
  uint8_t unit_sys_type_ = SYS_TYPE_UNKNOWN;
  uint8_t command_sys_type_override_ = 0;

  // Writes a packet, rewriting its system type byte (and checksum) when the
  // unit needs a different one than the template carries.
  void write_packet_(const uint8_t *data, uint8_t len);

  // True for packets that another controller (a real NaviLink) sends to the
  // unit, or specifically its NAVILINK_PRESENT. Ignores the system type byte.
  bool is_from_other_controller_() const;
  bool is_navilink_present_() const;

  // Transmit only into a quiet bus (upstream PR #82). Writing the instant a
  // packet ends can land on top of the unit's next packet - combi/boiler units
  // send their gas and water packets back to back - and the command is lost
  // without any error. receive() arms a reply when a packet ends;
  // transmit_if_quiet_() sends it once the bus has been silent for TX_QUIET_MS.
  static constexpr uint32_t TX_QUIET_MS = 20;  // ~40 byte-times at 19200 baud
  // If the bus never goes quiet this long after a reply was armed (e.g. a
  // busy cascade bus), reply straight after a packet as before.
  static constexpr uint32_t TX_FALLBACK_MS = 250;
  // A frame still incomplete after this much silence is abandoned.
  static constexpr uint32_t FRAME_TIMEOUT_MS = 100;
  // A reply this long after the unit's last packet ended is skipped: the unit's
  // next packet may be starting, and its first bytes can still sit in the UART
  // FIFO where available() doesn't see them. Happens when the main loop stalls
  // (e.g. while every sensor is published). Replies normally go out 20-40 ms
  // after a packet; on an NFC-250 the next packet follows ~195 ms after one ends.
  static constexpr uint32_t TX_LATE_MS = 100;
  // After this many skipped slots in a row, reply late rather than not at all.
  static constexpr uint8_t TX_MAX_LATE_SKIPS = 3;
  void arm_reply_();
  void transmit_if_quiet_();
  void send_reply_(bool in_other_controller_slot);
  bool     tx_armed_ = false;                // a packet ended; a reply slot is owed
  uint32_t tx_armed_ms_ = 0;                 // when that reply was armed
  bool     tx_after_other_controller_ = false;  // another NaviLink spoke since then
  uint32_t last_rx_ms_ = 0;                  // last time bytes were seen on the bus
  // Earliest time the last complete packet may have ended. receive() can't see
  // when bytes arrived, only that they were there by this call, so a packet
  // read in one go after a loop stall is dated back to the previous call.
  uint32_t last_frame_ms_ = 0;
  uint32_t poll_ms_ = 0;                     // when receive() last ran
  uint32_t prev_poll_ms_ = 0;                // when it ran before that
  uint8_t  late_skips_ = 0;                  // slots skipped in a row (TX_LATE_MS)

  // Rate limit for warnings about malformed or unexpected frames.
  static constexpr uint32_t LOG_THROTTLE_MS = 60000;
  uint32_t last_bad_len_log_ms_ = 0;
  uint32_t last_unknown_log_ms_ = 0;
  bool bad_len_logged_ = false;
  bool unknown_logged_ = false;
  // Short status packets are a property of the model: reported once per type.
  bool short_water_logged_ = false;
  bool short_gas_logged_ = false;

  // Raw capture (see set_raw_capture()). Bytes skipped while looking for a
  // frame marker are collected in junk_ and logged in chunks.
  bool raw_capture_ = false;
  uint8_t junk_[32];
  uint8_t junk_len_ = 0;
  bool raw_log_enabled_() const;
  void log_raw_(const char *what, const uint8_t *data, size_t len);
  void note_junk_(uint8_t byte);
  void flush_junk_();
  bool should_log_(uint32_t &last_ms, bool &logged_once);
};

  
}  // namespace navien
}  // namespace esphome
