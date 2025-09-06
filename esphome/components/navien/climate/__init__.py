

from esphome.const import CONF_ID
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate

from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

navien_ns = cg.esphome_ns.namespace("navien")

DEPENDENCIES = ["climate"]

NavienClimate = navien_ns.class_("NavienClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(NavienClimate)
    .extend(
        {
            cv.Required(CONF_ID): cv.declare_id(NavienClimate),
            cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    );

async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_parent(paren))
    
