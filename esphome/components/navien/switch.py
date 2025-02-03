print ("Hello compojnents switch")

from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.components.navien.sensor import NAVIEN_CONFIG_ID, Navien

AUTO_LOAD = ["switch", "sensor"]

#print (switch)

  
#import navien_ns, CONFIG_NAVIEN_ID, NAVIEN_NAMESPACE, Navien
#navien_ns = cg.esphome_ns.namespace("navien")
#import navien_ns

navien_ns = cg.esphome_ns.namespace("navien")

print ("Hello compojnents switch - ns")

#DEPENDENCIES = ["sensor.navien"]

#navien_ns = cg.esphome_ns.namespace(NAVIEN_NAMESPACE)

NavienOnOff = navien_ns.class_("NavienOnOffSwitch", switch.Switch, cg.Component)



CONFIG_SCHEMA = (
    switch.switch_schema(NavienOnOff)
    .extend(
        {
            cv.GenerateID(NAVIEN_CONFIG_ID): cv.use_id(Navien),
            #cv.GenerateID(): cv.declare_id(NavienOnOff)
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    
    paren = await cg.get_variable(config[NAVIEN_CONFIG_ID])
    cg.add(var.set_parent(paren))
    
