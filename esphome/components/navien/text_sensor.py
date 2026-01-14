import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["text_sensor", "sensor"]

CONF_HEATING_MODE = "heating_mode"
CONF_DEVICE_TYPE = "device_type"
CONF_OPERATING_STATE = "operating_state"
CONF_PANEL_VERSION = "panel_version"
CONF_CONTROLLER_VERSION = "controller_version"

navien_ns = cg.esphome_ns.namespace("navien")
NavienTextSensor = navien_ns.class_("NavienTextSensor", cg.PollingComponent)

CONFIG_SCHEMA = (
    cv.Schema({
            cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
            
            cv.Optional(CONF_HEATING_MODE): cv.boolean,
            
            cv.Optional(CONF_DEVICE_TYPE): cv.boolean,
            
            cv.Optional(CONF_OPERATING_STATE): cv.boolean,
            
            cv.Optional(CONF_PANEL_VERSION): cv.boolean,
            
            cv.Optional(CONF_CONTROLLER_VERSION): cv.boolean,
            cv.Optional("src"): cv.string
        }
    )
)

async def to_code(config):
    print ("To code")
    var = await text_sensor.new_text_sensor(config)
    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    
    if config.get(CONF_HEATING_MODE, False):
        cg.add(var.set_icon("mdi:autorenew"))
        cg.add(paren.set_heating_mode_sensor(var))
        
    if config.get(CONF_DEVICE_TYPE, False):
        cg.add(var.set_icon("mdi:chip"))
        cg.add(paren.set_device_type_sensor(var))
        
    if config.get(CONF_OPERATING_STATE, False):
        cg.add(var.set_icon("mdi:information-outline"))
        cg.add(paren.set_operating_state_sensor(var))
        
    if config.get(CONF_PANEL_VERSION, False):
        cg.add(var.set_icon("mdi:monitor-dashboard"))
        cg.add(paren.set_panel_version_sensor(var))
        
    if config.get(CONF_CONTROLLER_VERSION, False):
        cg.add(var.set_icon("mdi:chip"))
        cg.add(paren.set_controller_version_sensor(var))