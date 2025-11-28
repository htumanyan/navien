# ESPHome Module for Navien
This folder contains a working implementation of Navien tankless water heater protocols in a form of an ESPHome module.

## Supported capabilities
* Reading heater parameters - termperature values, water and gas usage etc.
* In progress - sending commands to start/stop and hot button


## Usage
Compile and upload as usual but don't forget to put AP names and passwords in secrets.yaml file

```
 cd <project_dir>/navien

 cp secrets.yaml.sample secrets.yaml

 esphome compile navien.yml

 esphome run navien.yml
```

### Real-time updates
Set `real_time: true` under the `navien` sensor in your YAML (enabled in the sample configs) to publish sensor updates as soon as packets arrive instead of waiting for the polling interval.
