import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["text_sensor", "sensor"]

CONF_COMBI_MODE = "combi_mode"

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema()
    .extend(
        {
            cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
        }
    )
)

async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    
    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_icon("mdi:autorenew"))
    cg.add(paren.set_combi_mode_sensor(var))