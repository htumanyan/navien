import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ICON
from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["text_sensor", "sensor"]

CONF_HEATING_MODE = "heating_mode"
CONF_DEVICE_TYPE = "device_type"
CONF_OPERATING_STATE = "operating_state"
CONF_PANEL_VERSION = "panel_version"
CONF_CONTROLLER_VERSION = "controller_version"

_DEFAULT_ICONS = {
    CONF_HEATING_MODE: "mdi:autorenew",
    CONF_DEVICE_TYPE: "mdi:chip",
    CONF_OPERATING_STATE: "mdi:information-outline",
    CONF_PANEL_VERSION: "mdi:monitor-dashboard",
    CONF_CONTROLLER_VERSION: "mdi:chip",
}

def _set_default_icon(config):
    if CONF_ICON in config:
        return config
    for key, icon in _DEFAULT_ICONS.items():
        if config.get(key, False):
            config = config.copy()
            config[CONF_ICON] = icon
            return config
    return config

CONFIG_SCHEMA = cv.All(
    text_sensor.text_sensor_schema()
    .extend(
        {
            cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),

            cv.Optional(CONF_HEATING_MODE): cv.boolean,

            cv.Optional(CONF_DEVICE_TYPE): cv.boolean,

            cv.Optional(CONF_OPERATING_STATE): cv.boolean,

            cv.Optional(CONF_PANEL_VERSION): cv.boolean,

            cv.Optional(CONF_CONTROLLER_VERSION): cv.boolean,
        }
    ),
    _set_default_icon,
)

async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    
    if config.get(CONF_HEATING_MODE, False):
        cg.add(paren.set_heating_mode_sensor(var))

    if config.get(CONF_DEVICE_TYPE, False):
        cg.add(paren.set_device_type_sensor(var))

    if config.get(CONF_OPERATING_STATE, False):
        cg.add(paren.set_operating_state_sensor(var))

    if config.get(CONF_PANEL_VERSION, False):
        cg.add(paren.set_panel_version_sensor(var))

    if config.get(CONF_CONTROLLER_VERSION, False):
        cg.add(paren.set_controller_version_sensor(var))