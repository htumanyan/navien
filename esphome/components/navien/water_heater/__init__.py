from esphome.const import (
  CONF_ID,
  CONF_MIN_TEMPERATURE,
  CONF_MAX_TEMPERATURE,
)
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import water_heater
from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

DEPENDENCIES = ["water_heater"]

navien_ns = cg.esphome_ns.namespace("navien")

NavienWaterHeater = navien_ns.class_("NavienWaterHeater", water_heater.WaterHeater, cg.Component)

CONFIG_SCHEMA = cv.All(
    water_heater.water_heater_schema(NavienWaterHeater)
    .extend({
        cv.Required(CONF_ID): cv.declare_id(NavienWaterHeater),
        cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
        cv.Optional(CONF_MIN_TEMPERATURE): cv.temperature,
        cv.Optional(CONF_MAX_TEMPERATURE): cv.temperature,
    })
    .extend(cv.COMPONENT_SCHEMA)
);

async def to_code(config):
    var = await water_heater.new_water_heater(config)

    if (min_temp := config.get(CONF_MIN_TEMPERATURE)) is not None:
        cg.add(var.set_min_temperature(min_temp))

    if (max_temp := config.get(CONF_MAX_TEMPERATURE)) is not None:
        cg.add(var.set_max_temperature(max_temp))

    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_parent(paren))

    await cg.register_component(var, config)

