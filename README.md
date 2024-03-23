# Navien Tankless Heather Protocol

This repository captures reverse engineered description of the low level details on communication between Navien tankless water heaters and the Navilink WiFi lite device.

## High Level Details

Navien supports several types of remote control devices. The older generation, such as NR-21DU and similar connect with two wires that are used to provide power to the remote device and also for the data exchange. Newer and more complex NaviLink devices (NaviLink and NaviLink Lite) connect over RS-485 connector. The documentation refers to power provided over separate pins/wires but my heater did not. I had to connect a separate 12V power supply to the NaviLink lite that was used to capture the traces, analyzed here.

![image](doc/Navien.png)

## Protocols
### [RS485 Connection](/doc/rs485.md)
### [2-Wire Connection](/doc/2-wire.md)