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
    DEVICE_CLASS_GAS,

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
UNIT_BTU  = "BTU"

CONF_INLET_TEMPERATURE  = "inlet_temperature"
CONF_GAS_INLET_TEMPERATURE  = "gas_inlet_temperature"
CONF_OUTLET_TEMPERATURE = "outlet_temperature"
CONF_GAS_OUTLET_TEMPERATURE = "gas_outlet_temperature"
CONF_GAS_SET_TEMPERATURE    = "gas_target_temperature"
CONF_WATER_FLOW         = "water_flow"
CONF_WATER_UTILIZATION  = "water_utilization"
CONF_GAS_TOTAL          = "gas_total"
CONF_GAS_CURRENT        = "gas_current"
CONF_SH_OUTLET_TEMPERATURE = "sh_outlet_temperature"
CONF_SH_RETURN_TEMPERATURE = "sh_return_temperature"
CONF_CONN_STATUS        = "conn_status"
CONF_REAL_TIME          = "real_time"

CONF_RECIRC_STATUS      = "recirc_status"
CONF_UNK_W06            = "unk_w06"
CONF_UNK_W07            = "unk_w07"
CONF_UNK_W10            = "unk_w10"
CONF_UNK_W14            = "unk_w14"
CONF_UNK_W15            = "unk_w15"
CONF_UNK_W17            = "unk_w17"
CONF_UNK_W20            = "unk_w20"
CONF_UNK_W21            = "unk_w21"
CONF_UNK_W22            = "unk_w22"
CONF_UNK_W23            = "unk_w23"
CONF_UNK_W24            = "unk_w24"
CONF_UNK_W25            = "unk_w25"
CONF_UNK_W26            = "unk_w26"
CONF_UNK_W27            = "unk_w27"
CONF_UNK_W28            = "unk_w28"
CONF_UNK_W29            = "unk_w29"
CONF_UNK_W30            = "unk_w30"
CONF_UNK_W31            = "unk_w31"
CONF_UNK_W32            = "unk_w32"
CONF_UNK_W34            = "unk_w34"
CONF_UNK_W35            = "unk_w35"
CONF_UNK_W36            = "unk_w36"
CONF_UNK_W37            = "unk_w37"
CONF_UNK_W38            = "unk_w38"
CONF_UNK_W39            = "unk_w39"
CONF_UNK_G00            = "unk_g00"
CONF_UNK_G01            = "unk_g01"
CONF_UNK_G02            = "unk_g02"
CONF_UNK_G03            = "unk_g03"
CONF_UNK_G18            = "unk_g18"
CONF_UNK_G19            = "unk_g19"
CONF_UNK_G20            = "unk_g20"

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
            cv.Optional(CONF_GAS_SET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),  
            
            cv.Optional(CONF_INLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_GAS_INLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_OUTLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_GAS_OUTLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),  
            cv.Optional(CONF_WATER_FLOW): sensor.sensor_schema(
                unit_of_measurement=UNIT_LPM,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_WATER_UTILIZATION): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_GAS_TOTAL): sensor.sensor_schema(
                unit_of_measurement=UNIT_CUBIC_METER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_GAS,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_GAS_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_BTU,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_SH_OUTLET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_SH_RETURN_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W06): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W07): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
         
            cv.Optional(CONF_UNK_W10): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W14): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W15): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W17): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W20): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W21): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W22): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W23): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W24): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W25): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W26): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W27): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W28): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W29): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W30): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W31): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W32): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W34): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W35): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W36): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W37): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W38): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_W39): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_G00): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_G01): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_G02): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_G03): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_G18): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_G19): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_UNK_G20): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=2,
            ),   
            cv.Optional(CONF_CONN_STATUS): binary_sensor.binary_sensor_schema(
                device_class = DEVICE_CLASS_CONNECTIVITY
            ),
            cv.Optional(CONF_RECIRC_STATUS): binary_sensor.binary_sensor_schema(),
            cv.Optional(CONF_REAL_TIME): cv.boolean
        }
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
    
    if CONF_GAS_SET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_GAS_SET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_GAS_SET_TEMPERATURE].get(CONF_ICON, "mdi:fire")))
        cg.add(var.set_gas_target_temp_sensor(sens))
        
    if CONF_INLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_INLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_INLET_TEMPERATURE].get(CONF_ICON, "mdi:water-thermometer")))
        cg.add(var.set_inlet_temp_sensor(sens))
        
    if CONF_GAS_INLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_GAS_INLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_GAS_INLET_TEMPERATURE].get(CONF_ICON, "mdi:fire-water")))
        cg.add(var.set_gas_inlet_temp_sensor(sens))
        
    if CONF_OUTLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_OUTLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_OUTLET_TEMPERATURE].get(CONF_ICON, "mdi:water-thermometer-outline")))
        cg.add(var.set_outlet_temp_sensor(sens))
        
    if CONF_GAS_OUTLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_GAS_OUTLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_GAS_OUTLET_TEMPERATURE].get(CONF_ICON, "mdi:fire-water-outline")))
        cg.add(var.set_gas_outlet_temp_sensor(sens))

    if CONF_WATER_FLOW in config:
        sens = await sensor.new_sensor(config[CONF_WATER_FLOW])
        cg.add(sens.set_icon(config[CONF_WATER_FLOW].get(CONF_ICON, "mdi:gauge")))
        cg.add(var.set_water_flow_sensor(sens))

    if CONF_WATER_UTILIZATION in config:
        sens = await sensor.new_sensor(config[CONF_WATER_UTILIZATION])
        cg.add(sens.set_icon(config[CONF_WATER_UTILIZATION].get(CONF_ICON, "mdi:water-percent")))
        cg.add(var.set_water_utilization_sensor(sens))
        
    if CONF_GAS_TOTAL in config:
        sens = await sensor.new_sensor(config[CONF_GAS_TOTAL])
        cg.add(var.set_gas_total_sensor(sens))

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
    
    if CONF_SH_OUTLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_SH_OUTLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_SH_OUTLET_TEMPERATURE].get(CONF_ICON, "mdi:thermometer-lines")))
        cg.add(var.set_sh_outlet_temp_sensor(sens))
    if CONF_SH_RETURN_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_SH_RETURN_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_SH_RETURN_TEMPERATURE].get(CONF_ICON, "mdi:thermometer-lines")))
        cg.add(var.set_sh_return_temp_sensor(sens))
    if CONF_UNK_W06 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W06])
        cg.add(var.set_unk_sensor_w06(sens))
    if CONF_UNK_W07 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W07])
        cg.add(var.set_unk_sensor_w07(sens))
 
    if CONF_UNK_W10 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W10])
        cg.add(var.set_unk_sensor_w10(sens))
    if CONF_UNK_W14 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W14])
        cg.add(var.set_unk_sensor_w14(sens))
    if CONF_UNK_W15 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W15])
        cg.add(var.set_unk_sensor_w15(sens))
    if CONF_UNK_W17 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W17])
        cg.add(var.set_unk_sensor_w17(sens))
    if CONF_UNK_W20 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W20])
        cg.add(var.set_unk_sensor_w20(sens))
    if CONF_UNK_W21 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W21])
        cg.add(var.set_unk_sensor_w21(sens))
    if CONF_UNK_W22 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W22])
        cg.add(var.set_unk_sensor_w22(sens))
    if CONF_UNK_W23 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W23])
        cg.add(var.set_unk_sensor_w23(sens))
    if CONF_UNK_W24 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W24])
        cg.add(var.set_unk_sensor_w24(sens))
    if CONF_UNK_W25 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W25])
        cg.add(var.set_unk_sensor_w25(sens))
    if CONF_UNK_W26 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W26])
        cg.add(var.set_unk_sensor_w26(sens))
    if CONF_UNK_W27 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W27])
        cg.add(var.set_unk_sensor_w27(sens))
    if CONF_UNK_W28 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W28])
        cg.add(var.set_unk_sensor_w28(sens))
    if CONF_UNK_W29 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W29])
        cg.add(var.set_unk_sensor_w29(sens))
    if CONF_UNK_W30 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W30])
        cg.add(var.set_unk_sensor_w30(sens))
    if CONF_UNK_W31 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W31])
        cg.add(var.set_unk_sensor_w31(sens))
    if CONF_UNK_W32 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W32])
        cg.add(var.set_unk_sensor_w32(sens))
    if CONF_UNK_W34 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W34])
        cg.add(var.set_unk_sensor_w34(sens))
    if CONF_UNK_W35 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W35])
        cg.add(var.set_unk_sensor_w35(sens))
    if CONF_UNK_W36 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W36])
        cg.add(var.set_unk_sensor_w36(sens))
    if CONF_UNK_W37 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W37])
        cg.add(var.set_unk_sensor_w37(sens))
    if CONF_UNK_W38 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W38])
        cg.add(var.set_unk_sensor_w38(sens))
    if CONF_UNK_W39 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_W39])
        cg.add(var.set_unk_sensor_w39(sens))
    if CONF_UNK_G00 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_G00])
        cg.add(var.set_unk_sensor_g00(sens))
    if CONF_UNK_G01 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_G01])
        cg.add(var.set_unk_sensor_g01(sens))
    if CONF_UNK_G02 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_G02])
        cg.add(var.set_unk_sensor_g02(sens))
    if CONF_UNK_G03 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_G03])
        cg.add(var.set_unk_sensor_g03(sens))

    if CONF_UNK_G18 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_G18])
        cg.add(var.set_unk_sensor_g18(sens))
    if CONF_UNK_G19 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_G19])
        cg.add(var.set_unk_sensor_g19(sens))
    if CONF_UNK_G20 in config:
        sens = await sensor.new_sensor(config[CONF_UNK_G20])
        cg.add(var.set_unk_sensor_g20(sens))