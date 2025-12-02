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


const uint8_t POWER_STATUS_ON_OFF_MASK     = 0x05;
const uint8_t RECIRCULATION_ON_OFF_MASK    = 0x20;


/**
 * Bitmask values for the sustem status byte in the water packet (WATER_DATA.system_status field)
 */
// If this bit is 1, then the display units are Celsius, otherwise - Farenheit
// On Navien 240A Celsius is set by turning DIP Switch 4 ON (upper position) on the front panel
// Note: this does not affect the values reported by Navien unit. Those are always reported in metric.
// This flag just tells us what unit does the Navien unit itself use for display on the front panel
const uint8_t SYS_STATUS_FLAG_UNITS      = 0x08;

// If this bit is 1, then recirculation is enabled.
// On Navien 240A Celsius is set by turning DIP Switch 2 ON (upper position) on the front panel
const uint8_t SYS_STATUS_FLAG_RECIRC     = 0x02;
  
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

//                                                                                                           temp  
const uint8_t SET_TEMP_CMD_TEMPLATE[] =  {PACKET_MARKER, 0x05, 0x0F, 0x50, 0x10, 0x0c, 0x4f, 0x00,   0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                                        //F7              05    0F    50    10    0C    4F    00      00       5E    00    00    00    00    00    00    00    00  84 
  
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
   *
   * HT added:
   * when the hot button is not present it is 0x25 and 0x05 depending on on/off status of the unit.
   * with the hot button present that bit flips to zero, leaving 0x05 (power on) and 0x00 (power off)
   */
  uint8_t system_power; 
  uint8_t unknown_10;
  uint8_t set_temp;
  uint8_t outlet_temp;
  uint8_t inlet_temp;
  uint8_t unknown_14;
  uint8_t unknown_15;
  uint8_t unknown_17;
  uint8_t operating_capacity;
  uint8_t water_flow;
  uint8_t unknown_20;
  uint8_t unknown_21;
  uint8_t unknown_22;
  uint8_t unknown_23;
  uint8_t sys_status; // 0x50 packet: serialBuffer[24]
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
  uint8_t unknown_27; 
  uint8_t unknown_28; // Counter A_lo 
  uint8_t unknown_29; // Counter A_hi
  uint8_t unknown_30; // Counter B_lo
  uint8_t unknown_31; // Counter B_hi
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
  uint8_t  unknown_19; /// decreasing counter when hot water is running
  uint8_t  unknown_20; /// 0x05 always so far
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
} NAVIEN_PACKET;

  
    
}  // namespace navien
}  // namespace esphome
