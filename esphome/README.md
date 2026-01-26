# ESPHome Module for Navien
This folder contains a working implementation of Navien tankless water heater protocols in a form of an ESPHome module.

## Supported capabilities
* Reading heater parameters - temperature values, water and gas usage etc.
* Sending commands to start/stop and hot button

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
