from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ICON, CONF_TYPE, ENTITY_CATEGORY_CONFIG

from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["switch", "sensor"]

navien_ns = cg.esphome_ns.namespace("navien")

NavienOnOff = navien_ns.class_("NavienOnOffSwitch", switch.Switch, cg.Component)
NavienAllowScheduledRecircSwitch = navien_ns.class_("NavienAllowScheduledRecircSwitch", switch.Switch, cg.Component)
NavienRawCaptureSwitch = navien_ns.class_("NavienRawCaptureSwitch", switch.Switch, cg.Component)

TYPE_POWER = "power"
TYPE_ALLOW_SCHEDULED_RECIRC = "allow_scheduled_recirc"
TYPE_RAW_CAPTURE = "raw_capture"


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
        # Logs every RS-485 frame at INFO (tag navien.raw) while on. Sends
        # nothing to the unit. Off after a reboot unless restore_mode says
        # otherwise, so a heavy log stream doesn't stay on by accident.
        TYPE_RAW_CAPTURE: switch.switch_schema(
            NavienRawCaptureSwitch,
            default_restore_mode="ALWAYS_OFF",
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:record-rec",
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
    
