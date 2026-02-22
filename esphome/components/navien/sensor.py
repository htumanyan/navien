import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, text_sensor, uart
from esphome.components import output
from esphome.core import ID  

NAVIEN_NAMESPACE = "navien"
NAVIEN_CONFIG_ID = "navien"

navien_ns = cg.esphome_ns.namespace(NAVIEN_NAMESPACE)

Navien = navien_ns.class_("Navien", cg.PollingComponent)
NavienLink = navien_ns.class_("NavienLink")


from esphome.const import (
    CONF_UART_ID,
    CONF_ID, UNIT_EMPTY, ICON_EMPTY,
    CONF_ICON,
    CONF_LATITUDE,
    CONF_LONGITUDE,
    CONF_SENSOR,
    CONF_NAME,
    CONF_TARGET_TEMPERATURE,
    
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_GAS,
    DEVICE_CLASS_RUNNING,
    
    ENTITY_CATEGORY_DIAGNOSTIC,
    
    STATE_CLASS_TOTAL_INCREASING,
    
    UNIT_CUBIC_METER,
    UNIT_DEGREES,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_HOUR
)


UNIT_LPM  = "l/m"
UNIT_BTU  = "BTU"

CONF_DHW_SET_TEMPERATURE = "dhw_set_temperature"
CONF_INLET_TEMPERATURE  = "inlet_temperature"
CONF_OUTLET_TEMPERATURE = "outlet_temperature"
CONF_WATER_FLOW         = "water_flow"
CONF_WATER_UTILIZATION  = "water_utilization"
CONF_GAS_TOTAL          = "gas_total"
CONF_GAS_CURRENT        = "gas_current"
CONF_SH_SET_TEMPERATURE = "sh_set_temperature"
CONF_SH_OUTLET_TEMPERATURE = "sh_outlet_temperature"
CONF_SH_RETURN_TEMPERATURE = "sh_return_temperature"
CONF_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_RECIRC_RUNNING = "recirc_running"

CONF_CONN_STATUS        = "conn_status"
CONF_REAL_TIME          = "real_time"
CONF_HEAT_CAPACITY      = "heat_capacity"
CONF_RECIRC_MODE        = "recirc_mode"
CONF_TOTAL_DHW_USAGE    = "total_dhw_usage"
CONF_TOTAL_OPERATING_TIME       = "total_operating_time"
CONF_BOILER_ACTIVE              = "boiler_active"
CONF_CUMULATIVE_DWH_USAGE_HOURS = "total_dhw_usage_hours"
CONF_CUMULATIVE_SH_USAGE_HOURS  = "total_sh_usage_hours"
CONF_DAYS_SINCE_INSTALL         = "days_since_install"
CONF_SRC                        = "src"
CONF_OTHER_NAVILINK_INSTALLED   = "other_navilink_installed"


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Navien),
            
            cv.Optional(CONF_NAME, default= 'Navien' ): cv.string_strict,

            cv.Optional(CONF_DHW_SET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
            ),

            # Left for backwards compatibility in config. Alias for DHW_SET_TEMPERATURE.
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
            cv.Optional(CONF_WATER_UTILIZATION): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_RECIRC_RUNNING): binary_sensor.binary_sensor_schema(
                device_class = DEVICE_CLASS_RUNNING
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
            cv.Optional(CONF_SH_SET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
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
            cv.Optional(CONF_OUTDOOR_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                icon="mdi:sun-thermometer",
            ),
            cv.Optional(CONF_HEAT_CAPACITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_TOTAL_DHW_USAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_TOTAL_OPERATING_TIME): sensor.sensor_schema(
                unit_of_measurement=UNIT_HOUR,
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_CUMULATIVE_DWH_USAGE_HOURS): sensor.sensor_schema(
                unit_of_measurement=UNIT_HOUR,
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_CUMULATIVE_SH_USAGE_HOURS): sensor.sensor_schema(
                unit_of_measurement=UNIT_HOUR,
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_DAYS_SINCE_INSTALL): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_BOILER_ACTIVE): binary_sensor.binary_sensor_schema(),
            
            cv.Optional(CONF_CONN_STATUS): binary_sensor.binary_sensor_schema(
                device_class = DEVICE_CLASS_CONNECTIVITY
            ),
            cv.Optional(CONF_RECIRC_MODE): text_sensor.text_sensor_schema(),
            cv.Optional(CONF_OTHER_NAVILINK_INSTALLED): binary_sensor.binary_sensor_schema(
                device_class = DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC
            ),
            cv.Optional(CONF_REAL_TIME): cv.boolean,
            cv.Optional(CONF_SRC): cv.int_range(min=0, max=15)
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)




async def to_code(config):
    # Create the Navien instance
    var = cg.new_Pvariable(config[CONF_ID])
    # Set UART interface and src index for NavienBase
    uart_id = config.get(CONF_UART_ID)
    if uart_id is not None:
        uart_var = await cg.get_variable(uart_id)
        cg.add(var.set_uart(uart_var))
    src = 0
    if CONF_SRC in config:
        src = config[CONF_SRC]
    cg.add(var.set_src(src))
    await cg.register_component(var, config)

    dhw_set_temp_config_key = None
    if CONF_DHW_SET_TEMPERATURE in config:
        dhw_set_temp_config_key = CONF_DHW_SET_TEMPERATURE

    # Backwards compat alias
    if CONF_TARGET_TEMPERATURE in config:
        if dhw_set_temp_config_key:
            raise cv.Invalid(f"{CONF_TARGET_TEMPERATURE} is deprecated. Use only {CONF_DHW_SET_TEMPERATURE}.")
        dhw_set_temp_config_key = CONF_TARGET_TEMPERATURE

    if dhw_set_temp_config_key:
        sens = await sensor.new_sensor(config[dhw_set_temp_config_key])
        cg.add(sens.set_icon(config[dhw_set_temp_config_key].get(CONF_ICON, "mdi:coolant-temperature")))
        cg.add(var.set_dhw_set_temp_sensor(sens))
    
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

    if CONF_RECIRC_RUNNING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_RECIRC_RUNNING])
        cg.add(sens.set_icon(config[CONF_RECIRC_RUNNING].get(CONF_ICON, "mdi:water-sync")))
        cg.add(var.set_recirc_running_sensor(sens))
        
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
        
    if CONF_RECIRC_MODE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_RECIRC_MODE])
        cg.add(var.set_recirc_mode_sensor(sens))
    
    if CONF_BOILER_ACTIVE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_BOILER_ACTIVE])
        cg.add(var.set_boiler_active_sensor(sens))          

    if CONF_SH_SET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_SH_SET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_SH_SET_TEMPERATURE].get(CONF_ICON, "mdi:coolant-temperature")))
        cg.add(var.set_sh_set_temp_sensor(sens))

    if CONF_SH_OUTLET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_SH_OUTLET_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_SH_OUTLET_TEMPERATURE].get(CONF_ICON, "mdi:thermometer-lines")))
        cg.add(var.set_sh_outlet_temp_sensor(sens))
        
    if CONF_SH_RETURN_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_SH_RETURN_TEMPERATURE])
        cg.add(sens.set_icon(config[CONF_SH_RETURN_TEMPERATURE].get(CONF_ICON, "mdi:thermometer-lines")))
        cg.add(var.set_sh_return_temp_sensor(sens))

    if CONF_OUTDOOR_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_OUTDOOR_TEMPERATURE])
        cg.add(var.set_outdoor_temp_sensor(sens))
        
    if CONF_HEAT_CAPACITY in config:
        sens = await sensor.new_sensor(config[CONF_HEAT_CAPACITY])
        cg.add(sens.set_icon(config[CONF_HEAT_CAPACITY].get(CONF_ICON, "mdi:heat-wave")))
        cg.add(var.set_heat_capacity_sensor(sens))
        
    if CONF_TOTAL_DHW_USAGE in config:
        sens = await sensor.new_sensor(config[CONF_TOTAL_DHW_USAGE])
        cg.add(sens.set_icon(config[CONF_TOTAL_DHW_USAGE].get(CONF_ICON, "mdi:water-boiler")))
        cg.add(var.set_total_dhw_usage_sensor(sens))
        
    if CONF_TOTAL_OPERATING_TIME in config:
        sens = await sensor.new_sensor(config[CONF_TOTAL_OPERATING_TIME])
        cg.add(sens.set_icon(config[CONF_TOTAL_OPERATING_TIME].get(CONF_ICON, "mdi:clock-outline")))
        cg.add(var.set_total_operating_time_sensor(sens))  
        
    if CONF_CUMULATIVE_DWH_USAGE_HOURS in config:
        sens = await sensor.new_sensor(config[CONF_CUMULATIVE_DWH_USAGE_HOURS])
        cg.add(sens.set_icon(config[CONF_CUMULATIVE_DWH_USAGE_HOURS].get(CONF_ICON, "mdi:clock-outline")))
        cg.add(var.set_cumulative_dwh_usage_hours_sensor(sens))  
        
    if CONF_CUMULATIVE_SH_USAGE_HOURS in config:
        sens = await sensor.new_sensor(config[CONF_CUMULATIVE_SH_USAGE_HOURS])
        cg.add(sens.set_icon(config[CONF_CUMULATIVE_SH_USAGE_HOURS].get(CONF_ICON, "mdi:clock-outline")))
        cg.add(var.set_cumulative_sh_usage_hours_sensor(sens))  
        
    if CONF_DAYS_SINCE_INSTALL in config:
        sens = await sensor.new_sensor(config[CONF_DAYS_SINCE_INSTALL])
        cg.add(sens.set_icon(config[CONF_DAYS_SINCE_INSTALL].get(CONF_ICON, "mdi:calendar-clock")))
        cg.add(var.set_days_since_install_sensor(sens))
 
    if CONF_OTHER_NAVILINK_INSTALLED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_OTHER_NAVILINK_INSTALLED])
        cg.add(var.set_other_navilink_installed_sensor(sens))
