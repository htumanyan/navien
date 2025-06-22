import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.components import output



print ("Hello compojnents navien")

#import CONFIG_NAVIEN_ID
#import navien_ns

NAVIEN_NAMESPACE = "navien"
NAVIEN_CONFIG_ID = "navien"

navien_ns = cg.esphome_ns.namespace(NAVIEN_NAMESPACE)

Navien = navien_ns.class_("Navien", cg.PollingComponent, uart.UARTDevice)
print ("Hello compojnents navien - imported")


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




#CONFIG_SCHEMA = (
#    sensor.sensor_schema(Navien, unit_of_measurement=UNIT_EMPTY, icon=ICON_EMPTY, accuracy_decimals=1,)
#    .extend(cv.polling_component_schema("60s"))
#    .extend(uart.UART_DEVICE_SCHEMA)
#)

UNIT_LPM = "l/m"
UNIT_KILOCALORIE = "kcal"
UNIT_CUBIC_METERS = "m^3"

CONF_INLET_TEMPERATURE  = "inlet_temperature"
CONF_OUTLET_TEMPERATURE = "outlet_temperature"
CONF_WATER_FLOW         = "water_flow"
CONF_CURRENT_GAS_USAGE      = "current_gas_usage"
CONF_ACCUMULATED_GAS_USAGE  = "accumulated_gas_usage"



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
            cv.Optional(CONF_CURRENT_GAS_USAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOCALORIE,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_ACCUMULATED_GAS_USAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CUBIC_METERS,
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

    if CONF_CURRENT_GAS_USAGE in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_GAS_USAGE])
        cg.add(var.set_current_gas_sensor(sens))

    if CONF_ACCUMULATED_GAS_USAGE in config:
        sens = await sensor.new_sensor(config[CONF_ACCUMULATED_GAS_USAGE])
        cg.add(var.set_accumulated_gas_sensor(sens))


print ("Finished compojnents navien")

