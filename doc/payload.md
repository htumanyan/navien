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

Gas and water packets are differentiated by the value at offset 3 in the header

```
+---------------------------------------------------
| F7 | Unknown Byte | Direction | Packet Type | ....
+---------------------------------------------------

F7 - the fixed marker of a beginning of the packet

Unknown byte** - always 0x5, likely protocol version

Direction** - 0x50 for reporting packets

Packet Type:
	0x50 - water
	0x0F - gas
```

### Sample Water Header

```
+---------------------------------------------------
| F7 | 0x50 | 0x50 | 0x50 | ....
+---------------------------------------------------
```

### Sample Gas Header

```
+---------------------------------------------------
| F7 | 0x50 | 0x50 | 0x0F | ....
+---------------------------------------------------
```


