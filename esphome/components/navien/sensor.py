import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, uart
from esphome.components import output



#print ("Hello compojnents navien")

#import CONFIG_NAVIEN_ID
#import navien_ns

NAVIEN_NAMESPACE = "navien"
NAVIEN_CONFIG_ID = "navien"

navien_ns = cg.esphome_ns.namespace(NAVIEN_NAMESPACE)

Navien = navien_ns.class_("Navien", cg.PollingComponent, uart.UARTDevice)
#print ("Hello compojnents navien - imported")


from esphome.const import (
    CONF_ID, UNIT_EMPTY, ICON_EMPTY,
    CONF_ICON,
    CONF_LATITUDE,
    CONF_LONGITUDE,
    CONF_SENSOR,
    CONF_NAME,
    CONF_TARGET_TEMPERATURE,

    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_WATER,
    DEVICE_CLASS_GAS,

    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,

    UNIT_CUBIC_METER,
    UNIT_DEGREES,
    UNIT_CELSIUS,
    UNIT_PERCENT
)




#CONFIG_SCHEMA = (
#    sensor.sensor_schema(Navien, unit_of_measurement=UNIT_EMPTY, icon=ICON_EMPTY, accuracy_decimals=1,)
#    .extend(cv.polling_component_schema("60s"))
#    .extend(uart.UART_DEVICE_SCHEMA)
#)

UNIT_LPM  = "l/m"
UNIT_BTU_PER_HOUR = "BTU"

CONF_INLET_TEMPERATURE  = "inlet_temperature"
CONF_OUTLET_TEMPERATURE = "outlet_temperature"
CONF_WATER_FLOW         = "water_flow"
CONF_WATER_UTILIZATION  = "water_utilization"
CONF_SYS_STATUS         = "sys_status"
CONF_GAS_TOTAL          = "gas_total"
CONF_GAS_TOTAL_CUBIC_FEET = "gas_total_cuft"
CONF_GAS_CURRENT        = "gas_current"

CONF_CONN_STATUS        = "conn_status"
CONF_REAL_TIME          = "real_time"

CONF_RECIRC_STATUS      = "recirc_status"



CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Navien),
            cv.Optional(CONF_NAME, default="Navien"): cv.string_strict,

            cv.Optional(CONF_TARGET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_INLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_OUTLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_WATER_FLOW): sensor.sensor_schema(
                unit_of_measurement=UNIT_LPM,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_WATER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_WATER_UTILIZATION): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=2,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SYS_STATUS): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon="mdi:information-outline",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_GAS_TOTAL): sensor.sensor_schema(
                unit_of_measurement=UNIT_CUBIC_METER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_GAS,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_GAS_TOTAL_CUBIC_FEET): sensor.sensor_schema(
                unit_of_measurement="ft³",
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_GAS,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_GAS_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_BTU_PER_HOUR,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CONN_STATUS): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY
            ),
            cv.Optional(CONF_RECIRC_STATUS): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_REAL_TIME): cv.boolean,
        },
        extra=cv.ALLOW_EXTRA,
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_TARGET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TARGET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_TARGET_TEMPERATURE].get(CONF_ICON, "mdi:coolant-temperature")))
        cg.add(var.set_target_temp_sensor(sens))
        
    if CONF_INLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_INLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_INLET_TEMPERATURE].get(CONF_ICON, "mdi:water-thermometer")))
        cg.add(var.set_inlet_temp_sensor(sens))
        
    if CONF_OUTLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_OUTLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_OUTLET_TEMPERATURE].get(CONF_ICON, "mdi:water-thermometer-outline")))
        cg.add(var.set_outlet_temp_sensor(sens))

    if CONF_WATER_FLOW in config:
        sens = await sensor.new_sensor(config[CONF_WATER_FLOW])
        cg.add(sens.set_icon(config[CONF_WATER_FLOW].get(CONF_ICON, "mdi:gauge")))
        cg.add(var.set_water_flow_sensor(sens))

    if CONF_WATER_UTILIZATION in config:
        sens = await sensor.new_sensor(config[CONF_WATER_UTILIZATION])
        cg.add(sens.set_icon(config[CONF_WATER_UTILIZATION].get(CONF_ICON, "mdi:water-percent")))
        cg.add(var.set_water_utilization_sensor(sens))

    if CONF_SYS_STATUS in config:
        sens = await sensor.new_sensor(config[CONF_SYS_STATUS])
        cg.add(var.set_sys_status_sensor(sens))

    if CONF_GAS_TOTAL in config:
        sens = await sensor.new_sensor(config[CONF_GAS_TOTAL])
        cg.add(var.set_gas_total_sensor(sens))

    if CONF_GAS_TOTAL_CUBIC_FEET in config:
        sens = await sensor.new_sensor(config[CONF_GAS_TOTAL_CUBIC_FEET])
        cg.add(var.set_gas_total_cuft_sensor(sens))

    if CONF_GAS_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_GAS_CURRENT])
        cg.add(sens.set_icon(config[CONF_GAS_CURRENT].get(CONF_ICON, "mdi:gas-burner")))
        cg.add(var.set_gas_current_sensor(sens))

    if CONF_REAL_TIME in config:
        cg.add(var.set_real_time(config[CONF_REAL_TIME]))

    if CONF_CONN_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CONN_STATUS])
        cg.add(var.set_conn_status_sensor(sens))
        
    if CONF_RECIRC_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_RECIRC_STATUS])
        cg.add(var.set_recirc_status_sensor(sens))
        
