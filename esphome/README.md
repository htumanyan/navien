# ESPHome Module for Navien
This folder contains a working implementation of Navien tankless water heater protocols in a form of an ESPHome module.

## Supported capabilities
* Reading heater parameters - temperature values, water and gas usage etc.
* Sending commands to start/stop and hot button
* Setting the space-heating (supply water) setpoint on combi / boiler units (verified on NCB-H) - see below

## Space-heating setpoint (combi / boiler units)

`send_sh_set_temp_cmd(deg_c)` sets the SH supply setpoint. There is no dedicated
platform yet; a template number works, and showing the unit's *reported* value
(not the last value typed) makes a rejected command visible:

```yaml
number:
  - platform: template
    name: "SH Setpoint"
    unit_of_measurement: "°F"
    device_class: temperature
    min_value: 104      # NCB-H SH range; the unit clamps anything outside it
    max_value: 180
    step: 1
    mode: box
    update_interval: 2s
    lambda: |-
      if (!id(navien_main).has_data()) return NAN;
      return id(navien_main).get_sh_set_temp_c() * 9.0f / 5.0f + 32.0f;
    set_action:
      - lambda: |-
          id(navien_main).send_sh_set_temp_cmd((x - 32.0f) * 5.0f / 9.0f);
```

The unit works in 0.5 °C steps (0.9 °F), so values round: 158 °F lands as 158.9.

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
