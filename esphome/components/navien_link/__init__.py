import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

AUTO_LOAD = ["sensor", "binary_sensor", "switch", "button", "text_sensor"]

NAVIEN_LINK_ID = "link_id"

print ("Hello namespace")
navien_ns = cg.esphome_ns.namespace("navien")
NavienLinkEsp = navien_ns.class_("NavienLinkEsp", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(NavienLinkEsp),
    })
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await uart.register_uart_device(var, config)
    await cg.register_component(var, config)