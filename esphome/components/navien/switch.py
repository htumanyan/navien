from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ICON, CONF_TYPE

from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["switch", "sensor"]

navien_ns = cg.esphome_ns.namespace("navien")

NavienOnOff = navien_ns.class_("NavienOnOffSwitch", switch.Switch, cg.Component)
NavienAllowScheduledRecircSwitch = navien_ns.class_("NavienAllowScheduledRecircSwitch", switch.Switch, cg.Component)
NavienUseGasTempsSwitch = navien_ns.class_("NavienUseGasTempsSwitch", switch.Switch, cg.Component)
NavienRealTimeSwitch = navien_ns.class_("NavienRealTimeSwitch", switch.Switch, cg.Component)

TYPE_POWER = "power"
TYPE_ALLOW_SCHEDULED_RECIRC = "allow_scheduled_recirc"
TYPE_USE_GAS_TEMPS = "use_gas_temps"
TYPE_REAL_TIME = "real_time"


CONFIG_SCHEMA = cv.typed_schema(
    {
        TYPE_POWER: switch.switch_schema(NavienOnOff)
        .extend(
            {
                cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        TYPE_ALLOW_SCHEDULED_RECIRC: switch.switch_schema(NavienAllowScheduledRecircSwitch)
        .extend(
            {
                cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
                cv.Optional(CONF_ICON, default="mdi:water-sync"): cv.icon,
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        TYPE_USE_GAS_TEMPS: switch.switch_schema(
            NavienUseGasTempsSwitch,
            default_restore_mode="RESTORE_DEFAULT_ON",
            icon="mdi:gas-burner",
        )
        .extend(
            {
                cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
        TYPE_REAL_TIME: switch.switch_schema(
            NavienRealTimeSwitch,
            default_restore_mode="RESTORE_DEFAULT_OFF",
            icon="mdi:flash",
        )
        .extend(
            {
                cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    },
    key=CONF_TYPE,
    default_type=TYPE_POWER,
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_parent(paren))
    
