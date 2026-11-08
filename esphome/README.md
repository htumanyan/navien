# ESPHome Module for Navien
This folder contains a working implementation of Navien tankless water heater protocols in a form of an ESPHome module.

## Supported capabilities
* Reading heater parameters - temperature values, water and gas usage etc.
* Sending commands to start/stop and hot button
* Setting the space-heating (supply water) setpoint on combi / boiler units (verified on NCB-H, NHB-H and NFC-250) - see below

The **Main Power** switch only sends a command when you flip it. It used to
send "power on" every time the ESP booted, which turned a unit that had been
switched off back on after a reboot or power cut.

## Combi boilers and boilers (NFC, NCB, NCB-H, NHB, NHB-H, NFB)

Add the `navien-combi.yml` package (or use
`navien-esphome-waveshare-esp32s3-rs485can-combi.yml`). It adds a
**Space Heating Setpoint** control and sets the DHW range for a combi. It
extends the `navien_climate` and `navien_water_heater` entities, so include it
after `navien-base.yml` and `navien-water-heater.yml`:

```yaml
substitutions:
  # Match your boiler AND your heating system (radiant floors need a much
  # lower SH maximum than radiators). Defaults are the NFC-H published ranges.
  dhw_min_temperature: 86F
  dhw_max_temperature: 140F
  sh_min_temperature: 77F
  sh_max_temperature: 185F

packages:
  # ... board, navien-base.yml, navien-sensors.yml, navien-water-heater.yml ...
  combi: !include navien-combi.yml
```

The setpoint control is the `navien` number platform:

```yaml
number:
  - platform: navien
    navien: navien_main
    name: "Space Heating Setpoint"
    min_value: 104F   # required
    max_value: 180F   # required
```

It shows the setpoint the unit reports (not the last value sent), so a
rejected or clamped command is visible, and it only sends once the unit has
identified itself as a model with space heating. The unit works in 0.5 °C
steps (0.9 °F), so values round: 158 °F lands as 158.9.

On these models the DHW sensors, water heater and climate entities read the
DHW values from the gas packet, and commands use the unit's own system type
byte. See [doc/combi_boilers.md](../doc/combi_boilers.md) for details, what's
verified per model, and how to capture frames from a model that isn't verified
yet (e.g. NCB, NFB). If commands still don't reach a new model, `command_sys_type`
on the `navien` sensor platform forces the header byte (e.g. `0x05`).

The Waveshare combi config also has a **Reset Reason** diagnostic sensor
(ESPHome `debug` component) that shows why the board last restarted: power
loss, watchdog, crash or an OTA update.

### Raw RS-485 capture

The `raw_capture` switch type logs every frame received or sent as one hex
line (log tag `navien.raw`) while it's on, without reflashing. On a
`logger: level: DEBUG` build the frames are always logged at DEBUG.

```yaml
switch:
  - platform: navien
    navien: navien_main
    type: raw_capture
    name: "Raw RS-485 Capture"
```

See [doc/combi_boilers.md](../doc/combi_boilers.md#capturing-frames-from-a-model-that-isnt-verified-yet).

## Requirements

The build script requires Python 3.11+ and one of:
- [uv](https://docs.astral.sh/uv/) (recommended)
- esphome installed via pip

## Usage

### Interactive build script (recommended)

```bash
cd esphome
uv run python build.py
```

The script will:
1. Prompt for WiFi credentials on first run (saved to `secrets.yaml`)
2. Let you select a hardware configuration
3. Choose an action (compile, upload, run, logs)
4. Copy firmware to `../build/` on successful compile

Your choices are persisted, so subsequent runs use your previous selections as defaults.

### Available configurations

| Config | Hardware |
|--------|----------|
| navien.yml | D1 Mini (ESP8266) - main config |
| navien-d1-mini.yml | D1 Mini variant |
| navien-esphome-atom-lite-esp32.yml | ESP32 Atom Lite |
| navien-esphome-atoms3-lite-tail485-esp32.yml | M5Stack AtomS3-Lite + Tail485 |
| navien-esphome-waveshare-esp32s3-rs485can.yml | Waveshare ESP32-S3-RS485-CAN |
| navien-esphome-waveshare-esp32s3-rs485can-combi.yml | Waveshare ESP32-S3-RS485-CAN on a combi boiler / boiler |
| navien-ht-device.yml | Custom HT device |
| navien-wrd-hb.yml | D1 Mini with hardwired hot button |

### Manual build

If you prefer to run esphome directly:

```bash
cd esphome
cp secrets.yaml.sample secrets.yaml
# Edit secrets.yaml with your WiFi credentials

esphome compile navien.yml
esphome run navien.yml
```

### Alternative installation methods

**Using pip:**
```bash
pip install esphome
esphome compile navien.yml
```

**Using Docker:**
```bash
docker run --rm -v "${PWD}":/config esphome/esphome compile navien.yml
```
