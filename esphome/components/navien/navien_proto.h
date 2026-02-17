/**
 * Copyright (c) 2024 Hovhannes Tumanyan (htumanyan)
 *
 * Packet formats and structures for Navien water heater control protocol
 */

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
   * System type - identifies the type of Navien system
   */
  uint8_t sys_type;

  /**
   * Source of the packet. Not unlikely to be "recipient address" essentially.
   * Needs to be captured in multi-unit installation to see if it is simply a direction or a version.
   *
   * Navien to control device - 0x50, PACKET_SRC_STATUS
   * Control Device to Navien - 0x0F, PACKET_SRC_CONTROL
   */
  uint8_t src;

  /**
   * Destination - there are two known packet types with somewhat overlapping
   * data but also with unique data points in each
   * 0x50 - water flow and temperature data - PACKET_DST_WATER
   * 0x0F - gas flow and also water temperature - PACKET_DST_GAS
   */
  uint8_t dst;

  /**
   * Direction of the packet.
   * 0x90 - Status packet (from Navien to control device), PACKET_DIR_STATUS
   * 0x10 - Control packet (from control device to Navien), PACKET_DIR_CONTROL
   */
  uint8_t direction;

  /**
   * Length of the packet including the header and checksum, i.e. total number of bytes
   * in the packet, including everything
   */
  uint8_t len;
} HEADER;

const uint8_t HDR_SIZE = sizeof(HEADER);
const uint8_t PACKET_MARKER = 0xF7;

const uint8_t PACKET_SRC_CONTROL = 0x0F;
const uint8_t PACKET_SRC_STATUS  = 0x50;

const uint8_t PACKET_DST_WATER = 0x50;
const uint8_t PACKET_DST_GAS   = 0x0F;

const uint8_t PACKET_DIR_STATUS  = 0x90;
const uint8_t PACKET_DIR_CONTROL = 0x10;

const uint16_t CHECKSUM_SEED_4B = 0x4b;
const uint16_t CHECKSUM_SEED_62 = 0x62;


const uint8_t POWER_STATUS_ON_OFF_MASK     = 0x01;
const uint8_t RECIRCULATION_ON_OFF_MASK    = 0x20;


/**
 * Bitmask values for the system status byte in the water packet (WATER_DATA.system_status field)
 */
// If this bit is 1, then the display units are Celsius, otherwise - Farenheit
// On Navien 240A Celsius is set by turning DIP Switch 4 ON (upper position) on the front panel
// Note: this does not affect the values reported by Navien unit. Those are always reported in metric.
// This flag just tells us what unit does the Navien unit itself use for display on the front panel
// NOTE: on a NCB unit this seems to be indicated in bit 1, not bit 3
const uint8_t SYS_STATUS_FLAG_UNITS      = 0x08;

// Recirculation mode bits in system_status:
// Bit 0 (0x01): Internal scheduled recirculation mode
// Bit 1 (0x02): External scheduled recirculation mode
// If either bit is set, the unit is in one of the scheduled modes, which cede control
// of recirculation scheduling to a NaviLink-like device (e.g. this ESPHome device) if one is present.
// If both bits are 0, recirculation is either disabled or in "HotButton" mode.
const uint8_t SYS_STATUS_FLAG_RECIRC_INT_SCHEDULED = 0x01;
const uint8_t SYS_STATUS_FLAG_RECIRC_EXT_SCHEDULED = 0x02;

/**
 * Bitmask values for the recirculation_enabled byte (WATER_DATA.recirculation_enabled field)
 */
// In HotButton mode: If this bit is 1, hot button recirculation has been triggered and is active
const uint8_t RECIRC_STATUS_FLAG_HOTBUTTON_ON = 0x01;
// In Scheduled mode: If this bit is 1, recirculation is enabled. Note that this can still be
// changed when in HotButton mode, but the unit doesn't actually do anything in response until you
// switch the unit back to Scheduled mode.
const uint8_t RECIRC_STATUS_FLAG_SCHEDULED_ON = 0x02;

/**
 * Bitmask values for GAS_DATA.system_status_2
 * This byte contains unit configuration flags that are consistent across unit types.
 */
// If this bit is 1, the unit is displaying imperial units (Fahrenheit), else metric (Celsius)
const uint8_t SYS_STATUS_2_DISPLAY_UNITS = 0x01;
// If this bit is 1, HotButton recirculation mode is enabled (vs disabled)
const uint8_t SYS_STATUS_2_HOTBUTTON_ENABLED = 0x04;

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

// When in scheduled recirculation mode, this enables recirculation. NaviLink sends this either when you disable a schedule (since that's equivalent to telling the 
// machine to always recirculate), or when the scheduled time block starts. It seems that when using a NaviLink, the schedule itself is kept either in the cloud or 
// on the NaviLink itself, and it just tells the water heater when to enable/disable recirculation.
// Once turned on, the unit will immediately run a recirculation cycle, and then continue to do so based on its internal logic until it receives the 
// SCHEDULED_RECIRC_OFF_CMD command.
// NOTE: if you send one of these commands while a real NaviLink is also connected, the NaviLink will immediately send its own command to reset the unit to whatever
// state it thinks it should be in!
const uint8_t SCHEDULED_RECIRC_ON_CMD[]  =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x00,   0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEE};
// When in scheduled recirculation mode, this turns off recirculation. NaviLink sends this either when you enable a schedule, or when the scheduled time block ends.
const uint8_t SCHEDULED_RECIRC_OFF_CMD[] =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x00,   0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0};

//                                                                                                           temp
const uint8_t SET_TEMP_CMD_TEMPLATE[] =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                                        //F7              05    0F    50    10    0C    4F    00      00       5E    00    00    00    00    00    00    00    00  84 

const uint8_t NAVILINK_PRESENT[]     =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x03, 0x4a, 0x00, 0x01, 0x55};

typedef struct {
  uint8_t unknown_06;
  uint8_t unknown_07;
  uint8_t heating_mode; // DEVICE_HEATING_MODE; 
  /**
   * on a combi like the NCB-H this is the state of the three-way valve setting DHW/SH heating. 
   * on models with recirc, this also indicates if the systems is running in recirc
   * values are enumerated in the DEVICE_HEATING_MODE enum
   * 0x00 - Idle
   * 0x08 - DHW Recirculating
   * 0x10 - Space Heating
   * 0x20 - Domestic Hot Water Demand
   * At least on an NPE-240A2 (maybe others), 0x08 only appears when the unit is in scheduled 
   * (not HotButton) recirculation mode and is actively recirculating. In HotButton mode, when 
   * a HotButton is pressed it shows as 0x20 here, but with recirculation_enabled=0x01 also set.
   */

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
   *
   * HT added:
   * when the hot button is not present it is 0x25 and 0x05 depending on on/off status of the unit.
   * with the hot button present that bit flips to zero, leaving 0x05 (power on) and 0x00 (power off)
   */
  uint8_t system_power;
  uint8_t operating_state;
  uint8_t set_temp;
  uint8_t outlet_temp;
  uint8_t inlet_temp;
  uint8_t unknown_14; //0x00 on NCB-H
  uint8_t unknown_15; //0x00 on NCB-H
  uint8_t unknown_16; //0x00 on NCB-H
  uint8_t operating_capacity;
  uint8_t water_flow;
  uint8_t unknown_19; //0x00 on NCB-H
  uint8_t unknown_20; //0x58 on NCB-H
  uint8_t unknown_21; //0xB1 on NCB-H
  uint8_t unknown_22; //0x10 on NCB-H
  uint8_t unknown_23; //0x89 on NCB-H
  /**
   * Kudos and credits to individuals below for this byte
   * tsquared at https://community.home-assistant.io/t/navien-esp32-navilink-interface/720567
   *
   *
   */
  uint8_t system_status;
  /**
   * Credit to dacarson for figuring out these values
   */
  uint8_t unknown_25;
  uint8_t unknown_26;
  uint8_t boiler_active; // Boiler Active Boolean
  uint8_t unknown_28; // Counter A_lo -- pinned to 255 on NCB-H models
  uint8_t unknown_29; // Counter A_hi -- pinned to 255 on NCB-H models
  uint8_t unknown_30; // Counter B_lo -- pinned to 255 on NCB-H models
  uint8_t unknown_31; // Counter B_hi -- pinned to 255 on NCB-H models
  uint8_t unknown_32;
  uint8_t recirculation_enabled;
  uint8_t unknown_34;
  uint8_t unknown_35;
  uint8_t unknown_36;
  uint8_t unknown_37;
  uint8_t unknown_38;
  uint8_t unknown_39;
} WATER_DATA;

typedef struct {
  uint8_t  unknown_06; // pinned to 0x45 on NCB-H models
  uint8_t  unknown_07; // 0x00
  uint8_t  device_type;
  uint8_t  unknown_09; //0x01 on NCB-H models
  uint8_t  controller_version_lo;
  uint8_t  controller_version_hi;

  uint8_t  panel_version_lo;
  uint8_t  panel_version_hi;

  uint8_t  set_temp;
  uint8_t  outlet_temp;
  uint8_t  inlet_temp;
  uint8_t  sh_outlet_temp; // combi (and space heat?) models
  uint8_t  sh_return_temp; // combi (and space heat?) models
  uint8_t  unknown_19;     // 0x9E on NCB-H models
  uint8_t  heat_capacity;  // varies based on boiler cycling while operating
  uint8_t  system_status_2;  // bit 5 always on on NCB-H models; bit 2 is hotbutton-enabled on NPE2 models; bit 0 is units on both
  uint8_t  current_gas_lo;
  uint8_t  current_gas_hi;
  uint8_t  cumulative_gas_lo;
  uint8_t  cumulative_gas_hi;
  uint8_t  unknown_26;                       // 0x00
  uint8_t  unknown_27;                       // 0x00
  uint8_t  days_since_install_lo;                       // Counter A_lo - total days since installation
  uint8_t  days_since_install_hi;                       // Counter A_hi
  uint8_t  cumulative_domestic_usage_cnt_lo; // 30 Domestic Usage Counter in 10 usage increments
  uint8_t  cumulative_domestic_usage_cnt_hi; // 31
  uint8_t  unknown_32;                       // 0x00 on NCB-H ; 0x9E 0x01 0xB7 0x46
  uint8_t  unknown_33;                       // Counter C_lo - Seems to match Water Counter A (/ 12.015)
  uint8_t  unknown_34;                       // Counter C_hi
  uint8_t  unknown_35;                       // 0x00
  uint8_t  total_operating_time_lo;          // 36
  uint8_t  total_operating_time_hi;          // 37
  uint8_t  cumulative_dwh_usage_hours_lo;                       // slow incrementing counter on NCB-H (lo)
  uint8_t  cumulative_dwh_usage_hours_hi;                       // 0x00 (hi)
  uint8_t  cumulative_sh_usage_hours_lo;      // 0x00 hourly incrementing counter on NCB-H when SH is running (lo)
  uint8_t  cumulative_sh_usage_hours_hi;      // 0x00 (hi)
  uint8_t  unknown_42;                       // 0x78 on NCB-H, 0xA6
  uint8_t  unknown_43;                       // 0x3C on NCB-H, 0x49
  uint8_t  unknown_44;                       // 0xA4 on NCB-H, 0x00
  uint8_t  unknown_45;                       // 0x50 on NCB-H, 0x00
  uint8_t  unknown_46;                       // 0x00 on NCB-H, 0x01
  uint8_t  unknown_47;                       // 0x00
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
} NAVIEN_PACKET;



}  // namespace navien
}  // namespace esphome
