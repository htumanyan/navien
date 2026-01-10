from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["switch", "sensor"]

navien_ns = cg.esphome_ns.namespace("navien")

NavienOnOff = navien_ns.class_("NavienOnOffSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = (
    switch.switch_schema(NavienOnOff)
    .extend(
        {
            cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    
    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_parent(paren))
    
