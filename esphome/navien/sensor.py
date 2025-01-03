import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_ID, UNIT_EMPTY, ICON_EMPTY,
    CONF_LATITUDE,
    CONF_LONGITUDE,
    CONF_SENSOR,
    CONF_NAME,
    CONF_TARGET_TEMPERATURE,

    UNIT_DEGREES,
    UNIT_CELSIUS,
)

DEPENDENCIES = ["uart"]


navien_sensor_ns = cg.esphome_ns.namespace("navien")
Navien = navien_sensor_ns.class_(
    "Navien", cg.PollingComponent, uart.UARTDevice
)


#CONFIG_SCHEMA = (
#    sensor.sensor_schema(Navien, unit_of_measurement=UNIT_EMPTY, icon=ICON_EMPTY, accuracy_decimals=1,)
#    .extend(cv.polling_component_schema("60s"))
#    .extend(uart.UART_DEVICE_SCHEMA)
#)

UNIT_LPM = "l/m"

CONF_INLET_TEMPERATURE  = "inlet_temperature"
CONF_OUTLET_TEMPERATURE = "outlet_temperature"
CONF_WATER_FLOW         = "water_flow"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Navien),
#            cv.Required(CONF_SENSOR): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_NAME, default= 'Navien' ): cv.string_strict,

            cv.Optional(CONF_TARGET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_INLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_OUTLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_WATER_FLOW): sensor.sensor_schema(
                unit_of_measurement=UNIT_LPM,
                accuracy_decimals=2,
            ),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
#    var = await sensor.new_sensor(config)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_TARGET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TARGET_TEMPERATURE])
        cg.add(var.set_target_temp_sensor(sens))
        
    if CONF_INLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_INLET_TEMPERATURE])
        cg.add(var.set_inlet_temp_sensor(sens))
        
    if CONF_OUTLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_OUTLET_TEMPERATURE])
        cg.add(var.set_outlet_temp_sensor(sens))

    if CONF_WATER_FLOW in config:
        sens = await sensor.new_sensor(config[CONF_WATER_FLOW])
        cg.add(var.set_water_flow_sensor(sens))
