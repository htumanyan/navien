# Packet Types
The Navien protocol has two primary types of packets:

* Reporting Packets: These are sent from the heater to the consumer device. They contain various pieces of information, including heater status, temperature readings, flow rate (GPM), gas usage values, and other operational data.

* Control Packets: These are sent from the controller device to the heater. They are responsible for issuing commands, such as adjusting temperature settings, activating specific features, or controlling the heater's operation.

## Reporting Packets
Reporting packets are sent by Navien water heaters to indicate the status of the device. There are two kinds of reporting packets:

* Water - shows temperature values and the water flow
* Gas - temperatue values and gas usage.

Interestingly, the temperature values in gas and water packets, while often similar, are not always identical. It is likely that these values represent temperatures measured at closely located points within the system.

This distinction suggests that the gas and water temperature readings serve specific purposes, reflecting measurements taken at different but closely connected stages of the heating process.

Gas and water status segments are selected by destination/service routing in header byte 0x03 (`dst`), not by a standalone packet-kind byte.

For full source/destination/direction routing details, see [Protocol Data Flow](./protocol_data_flow.md).

```
+---------------------------------------------------
| F7 | Unknown Byte | Src | Dst | Direction | ....
+---------------------------------------------------

F7 - the fixed marker of a beginning of the packet

Unknown byte** - observed as 0x05, likely protocol/version marker

Src** - source node ID (for example `0x50` main status source, `0x0F` local controller)

Dst** - destination/service routing byte (`0x50` water status path, `0x0F` gas status path)

Direction** - parser treats `0x90` as status path and `0x10` as control path
```

### Sample Water Header

```
+---------------------------------------------------
| F7 | 0x05 | 0x50 | 0x50 | 0x90 | ....
+---------------------------------------------------
```

### Sample Gas Header

```
+---------------------------------------------------
| F7 | 0x05 | 0x50 | 0x0F | 0x90 | ....
+---------------------------------------------------
```



## NHB-H water status fields (bytes 35 and 39)

Two bytes of the water status packet have been identified on an NHB-110H (device type 16) by comparing them with the unit's front panel. Offsets are packet offsets, counting the `F7` marker as byte 0. They are only exposed for NHB-H; other models have not been checked.

| Offset | Field | Scaling | How it was confirmed |
|---|---|---|---|
| 35 | `exhaust_temp` | whole °C | Matches the exhaust temperature shown on the front panel |
| 39 | `dhw_boiler_set_temp` | raw / 2 °C | Follows the installer setting "DHW Boiler Set Temp" (the boiler supply target while heating the indirect tank): changing it 82 → 81 → 82 °C on the panel changed the byte 0xA4 → 0xA2 → 0xA4 within a minute |

Example water frame from the same NHB-110H:

```
F7:04:50:50:90:22:42:00:00:17:14:76:80:82:00:00:00:00:00:00:00:64:20:00:02:04:00:00:24:00:18:00:00:C8:AA:2D:00:00:78:A4:38
                                                                                                         ^^          ^^
```

byte 35 = 0x2D = 45 °C (exhaust), byte 39 = 0xA4 = 82 °C (DHW boiler set).

The ESPHome keys are `exhaust_temperature` and `dhw_boiler_set_temperature`; both publish `NAN` on other device types.
