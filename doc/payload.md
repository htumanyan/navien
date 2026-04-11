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


