from esphome.components import button
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_ICON
from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

navien_ns = cg.esphome_ns.namespace("navien")
NavienHotButton = navien_ns.class_("NavienHotButton", button.Button, cg.Component)

AUTO_LOAD = ["sensor"]
CONFIG_SCHEMA = (
    button.button_schema(NavienHotButton)
    .extend(
        {
            cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
            cv.Optional(CONF_ICON, default="mdi:fire-circle"): cv.icon,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
    if CONF_ICON in config:
        cg.add(var.set_icon(config[CONF_ICON]))

    hub = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_parent(hub))
