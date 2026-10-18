# Combi boilers and boilers (NFC, NCB, NCB-H, NHB, NHB-H, NFB)

How combi and boiler units differ from tankless (NPE) units on the RS-485 bus,
what the component does about it, and what is still unknown.

Status per model:

| Model | Device type (gas byte 8) | System type (header byte 1) | Status |
|---|---|---|---|
| NPE (tankless) | 1 | `0x05` | Verified |
| NCB-H (combi) | 13 | `0x06` | Verified: reading, DHW + SH setpoint (upstream PR #81) |
| NHB-H (boiler) | 16 | `0x04` | Verified: reading, DHW + SH setpoint (upstream PRs #89, #91, #92) |
| NFC (fire-tube combi, e.g. NFC-250) | 8 | `0x06` | Verified on an NFC-250: reading, DHW + SH setpoint, power on/off (captures below). Same frame layout as the NCB-H |
| NCB, NHB, NFB | 2, 3, 6 (from the enum, not captured) | Not captured yet | Supported on the same rules; needs a capture to confirm |

Device-type codes follow the `DEVICE_TYPE` enum in `navien.h` (from
[PyNavienSmartControl](https://github.com/rudybrian/PyNavienSmartControl)).
An unmapped code shows as `Unknown (N)` on the Device Type sensor.

> **Note.** The Android "Navien" app (`com.navien.app`) is a product-lookup and
> support app (serial-number lookup, manuals, troubleshooting pages, AI chat).
> It has no device protocol, telemetry or control code, so it can't be used as
> a protocol reference. Version 1.7.0 was checked: no MQTT/NaviLink/setpoint
> strings or device-type tables in the Hermes bundle or the Java code.

## System type byte (header byte 1)

The second header byte is constant per model and appears in every packet the
unit sends:

```
F7 05 50 0F 90 ...   NPE   (tankless)
F7 06 50 0F 90 ...   NCB-H and NFC (combi)
F7 04 50 0F 90 ...   NHB-H (boiler)
```

Control packets we send must carry a value the unit accepts. The NCB-H
accepts the `0x05` used for tankless units, while the NHB-H ignores every
packet that doesn't carry its own `0x04` (upstream PR #91: no command applied
with `0x05`; 4 of 4 applied with `0x04`, together with the transmit timing fix
below).

Rule used by `NavienLink::command_sys_type()`:

| Unit reports | We send | Why |
|---|---|---|
| nothing yet | `0x05` | Unchanged default |
| `0x05` | `0x05` | Verified (NPE) |
| `0x06` | `0x05` | Verified (NCB-H, PR #81; NFC-250: DHW + SH setpoint, power) |
| anything else | the same value | Verified for `0x04` (NHB-H); assumed for models not captured yet |

The checksum covers byte 1, so it is recomputed (seed `0x62`). For example,
`NAVILINK_PRESENT`:

```
F7 05 0F 50 10 03 4A 00 01 55   (0x05 framing)
F7 04 0F 50 10 03 4A 00 01 D5   (0x04 framing, NHB-H)
```

The unit's value is logged once at INFO:

```
[I][navien.link]: Unit reports system type 0x04 (SRC:0x50); sending commands with system type 0x04
```

If commands still don't work on a new model, the `command_sys_type` option of
the `navien` sensor platform forces the value (e.g. `command_sys_type: 0x05`).
Leave it unset otherwise.

## Transmit timing

Combi/boiler units send their gas and water status packets back to back. A
command written the moment the first one ends lands on top of the second and is
lost with no error on either side (upstream PR #82, NCB-H; also needed on the
NHB-H). The link layer now arms a reply when a packet ends and writes it only
after the bus has been quiet for 20 ms (about 40 byte-times at 19200 baud).
What gets sent (a queued command, or `NAVILINK_PRESENT`) is decided as before.
If the bus never goes quiet for 250 ms (e.g. a busy cascade), it falls back
to replying straight after a packet, as it did before. A frame left
incomplete for 100 ms is dropped, so a corrupt header can't stall the receiver.

A reply can also come out too late. Every 5 s the component publishes all its
sensors, which can stall the main loop for 150-190 ms (more while a web page
or log stream is connected). On the NFC-250, whose packets follow each other
about 195 ms apart, a reply delayed that long lands on the start of the next
packet, and both get garbled: 3 of 4 checksum errors in a 10-minute capture
followed a reply sent more than 180 ms late. The first bytes of the new packet
can still be in the UART FIFO, where the quiet check can't see them. So a
reply more than 100 ms after the unit's last packet ended is skipped, and the
next packet gets one instead (a queued command waits for it). A packet read in
one go after a stall counts as ending at the previous poll, since it may have
been waiting in the buffer. After 3 skipped slots in a row the component
replies anyway, so a loop that is always late still reaches the unit.

On the NFC-250, with the same capture running:

| Firmware | Packets | Checksum errors | Latest reply | Slots skipped |
|---|---|---|---|---|
| Before (8 min) | 2454 | 4 (+1 dropped frame) | 222 ms | - |
| After (10 min) | 3000 | 0 | 80 ms | 65 |

## Where DHW and space-heating values come from

The gas packet (`dst 0x0F`) reports DHW and space-heating values side by side.
The water packet (`dst 0x50`) has one set of temperatures, and on an NHB-H
those belong to the heating loop rather than the DHW tank (upstream PR #92):

| Water-packet field | NHB-H value | Actually is |
|---|---|---|
| `outlet_temp` | 64 °C | SH supply (same as gas `sh_outlet_temp`) |
| `inlet_temp` | 65.5 °C | SH return |
| `dhw_set_temp` | 59 °C idle, 81 °C while heating the tank | Active supply target |

On the NFC-250 the water packet's set / outlet / inlet temperatures match the
gas packet's DHW values (see the capture below), so they are DHW values there.

For every model with space heating (`has_space_heating()` in `navien.h`:
NCB, NCB-H, NFC, NHB, NHB-H, NFB and the boiler cascades), the DHW Set /
Outlet / Inlet sensors and the water heater and climate entities use the gas
packet's DHW values. Before this change those sensors were published from both
packets and flipped between two different values on every update.
Tankless units still use the water packet, as before.

### Gas packet fields used for combi/boiler units

Offsets from the start of the frame (6-byte header):

| Offset | Field | Encoding |
|---|---|---|
| 8 | `device_type` | `DEVICE_TYPE` enum |
| 13 | `sh_set_temp` | °C × 2 |
| 14 | `dhw_set_temp` | °C × 2 |
| 15 | `outlet_temp` (DHW) | °C × 2 |
| 16 | `inlet_temp` (DHW) | °C × 2 |
| 17 | `sh_outlet_temp` (supply) | °C × 2 |
| 18 | `sh_return_temp` | °C × 2 |
| 19 | `outdoor_temp` | sign-magnitude whole °C, `0x9E` = no sensor |

Example, NCB-240/110H gas packet (first 40 of 49 bytes, from the
[HA community thread](https://community.home-assistant.io/t/navien-esp32-navilink-interface/720567/102)):

```
f7 06 50 0f 90 2a 45 00 0d 01 0a 06 0e 83 5a 33 2e 4d 46 9e 00 21 00 00 7b 43 00 00 5f 01 70 05 00 00 00 00 25 01 a7 04
```

| Bytes | Value | Decoded |
|---|---|---|
| 1 | `06` | System type (NCB-H) |
| 8 | `0d` | Device type 13 = NCB-H |
| 13 | `83` | SH setpoint 65.5 °C |
| 14 | `5a` | DHW setpoint 45 °C |
| 15 / 16 | `33` / `2e` | DHW outlet 25.5 °C / inlet 23 °C |
| 17 / 18 | `4d` / `46` | SH supply 38.5 °C / return 35 °C |
| 19 | `9e` | No outdoor sensor |

### NFC-250 capture

NFC-250 (Panel Version 1.4, Controller Version 1.0), idle, captured with the
Raw RS-485 Capture switch. The unit sends the gas packet and the water packet
about 200 ms apart, and we answer each with `NAVILINK_PRESENT`:

```
RX 66446: F7 06 50 0F 90 2A 45 00 08 01 0A 02 0E A4 67 5C 34 70 70 10 00 29 00 00 B0 5C 00 00 F3 01 55 0C 00 00 00 00 B1 01 13 06 AF 08 78 3C A4 50 00 00 D4
TX 66482: F7 05 0F 50 10 03 4A 00 01 55
RX 66641: F7 06 50 50 90 22 42 00 00 17 14 67 5C 34 00 00 00 00 00 00 F0 B9 10 00 00 04 00 00 FF FF FF FF 00 00 00 00 00 00 00 00 23
TX 66664: F7 05 0F 50 10 03 4A 00 01 55
```

Gas packet (49 bytes, same length and layout as the NCB-H):

| Bytes | Value | Decoded |
|---|---|---|
| 1 | `06` | System type, same as the NCB-H |
| 8 | `08` | Device type 8 = NFC |
| 10 / 12 | `0A` / `0E` | Controller 1.0 / Panel 1.4 |
| 13 | `A4` | SH setpoint 82 °C |
| 14 | `67` | DHW setpoint 51.5 °C |
| 15 / 16 | `5C` / `34` | DHW outlet 46 °C / inlet 26 °C |
| 17 / 18 | `70` / `70` | SH supply 56 °C / return 56 °C |
| 19 | `10` | Outdoor +16 °C |
| 28-29 | `F3 01` | Days since install, 499 |
| 30-31 | `55 0C` | DHW uses, 3157 |
| 36-37 | `B1 01` | 433, decoded as total operating time (see below) |
| 38-39 / 40-41 | `13 06` / `AF 08` | DHW hours 1555 / SH hours 2223 |
| 42-45 | `78 3C A4 50` | Unknown; identical on the NCB-H |

Water packet (41 bytes):

| Bytes | Value | Decoded |
|---|---|---|
| 8 | `00` | Heating mode: idle |
| 9 | `17` | System power, bit 0 = on |
| 10 | `14` | Operating state 20 = Standby |
| 11 / 12 / 13 | `67` / `5C` / `34` | DHW set 51.5 °C / outlet 46 °C / inlet 26 °C, same as the gas packet |
| 28-31 | `FF FF FF FF` | Pinned, as on the NCB-H |

DHW setpoint set to 54 °C from Home Assistant. The command goes out twice,
with `0x05` framing, and the next gas packet reports the new value about
1.5 s later:

```
TX 13460961: F7 05 0F 50 10 0C 4F 00 00 6C 00 00 00 00 00 00 00 00 52
TX 13461156: F7 05 0F 50 10 0C 4F 00 00 6C 00 00 00 00 00 00 00 00 52
RX 13462527: F7 06 50 0F 90 2A 45 00 08 01 0A 02 0E A4 6C ...   (byte 14 = 6C, 54.0 °C)
```

SH setpoint set to 80 °C with the `navien` number, applied about 2.5 s later:

```
TX 13641524: F7 05 0F 50 10 0C 4F 00 00 00 A0 00 00 00 00 00 00 00 1C
TX 13641747: F7 05 0F 50 10 0C 4F 00 00 00 A0 00 00 00 00 00 00 00 1C
RX 13644107: F7 06 50 0F 90 2A 45 00 08 01 0A 02 0E A0 ...   (byte 13 = A0, 80.0 °C)
```

Power off, then on again. The water packet's byte 9 (`system_power`) bit 0
follows about 2 s after each command:

```
TX 13920505: F7 05 0F 50 10 0C 4F 00 0B 00 00 00 00 00 00 00 00 00 0A   (off, sent twice)
RX 13922267: F7 06 50 50 90 22 42 00 00 16 ...                          (byte 9 = 16, off)
TX 13922504: F7 05 0F 50 10 0C 4F 00 0A 00 00 00 00 00 00 00 00 00 CE   (on, sent twice)
RX 13924665: F7 06 50 50 90 22 42 00 00 17 ...                          (byte 9 = 17, on)
```

A setpoint changed on the boiler's own panel shows up in the next status
packets with no control packet on the bus: the panel doesn't use this bus.

## Control packet (`0x4F`)

```
F7 05 0F 50 10 0C 4F 00 b8 b9 b10 b11 b12 00 00 00 00 00 <checksum, seed 0x62>
```

| Byte | Meaning | Values |
|---|---|---|
| 8 | Power | `0x0A` on, `0x0B` off, `0x00` no change |
| 9 | DHW setpoint | °C × 2, `0x00` no change |
| 10 | SH setpoint (combi/boiler) | °C × 2, `0x00` no change. Verified on NCB-H, NHB-H and NFC-250 |
| 11 | Hot button / recirculation | `0x01` hot button, `0x08` recirc on, `0x10` recirc off |
| 12 | Token | Varies; NaviLink sets it with recirculation commands |
| 13-17 | Unknown | `0x00` |

Example, SH setpoint 60 °C on a unit that needs `0x04` framing:

```
F7 04 0F 50 10 0C 4F 00 00 00 78 00 00 00 00 00 00 00 84
```

The `navien` number platform (`type: sh_set_temperature`) sends byte 10 and
shows the setpoint the unit reports back, so a rejected or clamped command is
visible. It refuses to send until the unit has identified itself as a model
with space heating.

## Not known yet

- **Turning space heating on or off** (summer/winter mode) over RS-485. No
  capture or source shows a command for it. Power off (byte 8 = `0x0B`) stops
  the whole unit, DHW included.
- Zone call / pump / 3-way-valve bits (upstream issue #50).
- NFB specifics: system type byte, whether the water packet's temperatures
  are DHW or heating loop.
- NFC operating-state and heating-mode codes other than idle / Standby (20).
- Gas bytes 36-37, decoded as total operating time: 433 on the NFC-250, lower
  than its DHW (1555) and SH (2223) hours. The NCB-H shows the same pattern,
  so the field is probably not total hours, or uses another unit.

### Published ranges, NFC-H (NFC-250/175H, NFC-250/200H)

From the [Navien product page](https://www.navieninc.com/products/nfc-250-200h):
DHW 86-140 °F (30-60 °C), SH supply 77-185 °F (25-85 °C), SH return
68-158 °F (20-70 °C). These are the defaults in `esphome/navien-combi.yml`.
NCB-H differs (SH 104-180 °F), and the unit clamps values outside its limits.

## Capturing frames from a model that isn't verified yet

The component can log every RS-485 frame it receives or sends, one hex line
per frame, under the `navien.raw` log tag:

```
[I][navien.raw]: RX 482113: F7 05 50 50 90 22 42 00 00 25 49 5C 5B 53 00 00 00 00 00 00 88 C2 00 20 02 00 00 00 21 03 99 08 00 02 00 00 00 00 00 00 87
[I][navien.raw]: TX 482152: F7 05 0F 50 10 03 4A 00 01 55
[I][navien.raw]: RX (between frames) 482310: 00 13 FF
```

(An NPE water status packet, our `NAVILINK_PRESENT` reply, and three stray
bytes.)

The number is the device uptime in ms, so the gaps between frames can be
measured. `RX (between frames)` holds bytes that weren't part of a frame
(noise, or data from a model that frames its packets differently).
`TX skipped (late)` marks a reply that was skipped because it would have gone
out too late (see Transmit timing). `RX (bad length, header only)` and
`RX (incomplete, header only)` show the header of a frame that was dropped
because its length byte was impossible or the rest never arrived. Lines are
logged before checksum checks, so corrupted frames show up too.

There are two ways to turn it on:

- **Raw RS-485 Capture switch** (no reflash, works on the default INFO build).
  `navien-combi.yml` includes it; for other configs add:

  ```yaml
  switch:
    - platform: navien
      navien: navien_main
      type: raw_capture
      name: "Raw RS-485 Capture"
  ```

  While it's on, frames are logged at INFO. It turns itself off after a
  reboot (set `restore_mode: RESTORE_DEFAULT_OFF` to keep it across reboots).
- **DEBUG build** (`logger: level: DEBUG`): frames are always logged, at
  DEBUG. To keep them but hide the other debug output, or the other way
  round:

  ```yaml
  logger:
    level: DEBUG
    logs:
      navien.sensor: INFO   # hide decoded-value debug lines
      navien.link: INFO
      # navien.raw: INFO    # or: hide the raw frames instead
  ```

To capture:

1. Turn capture on and save the log, e.g.
   `esphome logs navien-esphome-waveshare-esp32s3-rs485can-combi.yml | tee capture.txt`
   (or copy it from the ESPHome dashboard's log view).
2. Record the INFO lines `Unit reports system type 0x..` and `device type: ...`.
3. Capture a few minutes with the unit idle, during a hot-water draw and while
   space heating, plus one change of each setpoint from the unit's panel.
4. Note what the panel shows at the same time (setpoints, supply/return, DHW
   temperature), so the bytes can be matched to values.
5. Turn capture off again; it adds a few log lines per second.
6. Attach the log to an issue together with the model, front-panel photo and
   firmware versions (Controller/Panel Version sensors).
