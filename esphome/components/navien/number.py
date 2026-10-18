import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_STEP,
    CONF_TYPE,
    DEVICE_CLASS_TEMPERATURE,
    UNIT_CELSIUS,
)

from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["sensor"]

navien_ns = cg.esphome_ns.namespace("navien")
NavienShSetpointNumber = navien_ns.class_(
    "NavienShSetpointNumber", number.Number, cg.Component
)

TYPE_SH_SET_TEMPERATURE = "sh_set_temperature"


def _validate_range(config):
    if config[CONF_MIN_VALUE] >= config[CONF_MAX_VALUE]:
        raise cv.Invalid(f"{CONF_MIN_VALUE} must be lower than {CONF_MAX_VALUE}")
    return config


# Space-heating supply setpoint for combi / boiler units. The value is in
# deg C (Home Assistant converts it for display); min/max accept units, e.g.
# 104F. They are required because a safe supply temperature depends on the
# heating system (radiant floors need much lower limits than radiators).
CONFIG_SCHEMA = cv.typed_schema(
    {
        TYPE_SH_SET_TEMPERATURE: cv.All(
            number.number_schema(
                NavienShSetpointNumber,
                unit_of_measurement=UNIT_CELSIUS,
                device_class=DEVICE_CLASS_TEMPERATURE,
                icon="mdi:radiator",
            )
            .extend(
                {
                    cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
                    cv.Required(CONF_MIN_VALUE): cv.temperature,
                    cv.Required(CONF_MAX_VALUE): cv.temperature,
                    # The unit works in 0.5 deg C steps
                    cv.Optional(CONF_STEP, default=0.5): cv.positive_float,
                }
            )
            .extend(cv.COMPONENT_SCHEMA),
            _validate_range,
        ),
    },
    key=CONF_TYPE,
    default_type=TYPE_SH_SET_TEMPERATURE,
)


async def to_code(config):
    var = await number.new_number(
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=config[CONF_STEP],
    )
    await cg.register_component(var, config)

    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_parent(paren))
