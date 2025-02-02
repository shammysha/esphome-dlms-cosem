import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_TYPE
)
from . import (
    DlmsCosem,
    dlms_cosem_ns,
    obis_code,
    CONF_DLMS_COSEM_ID,
    CONF_DONT_PUBLISH
)

AUTO_LOAD = ["dlms_cosem"]

TYPE_ENUM = ["CONNECTION"]

DlmsCosemBinarySensor = dlms_cosem_ns.class_(binary_sensor.BinarySensor)

CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(
        BinarySensor,
    ).extend(
        {
            cv.GenerateID(CONF_DLMS_COSEM_ID): cv.use_id(DlmsCosem),
            cv.Required(CONF_TYPE): cv.enum(TYPE_ENUM, upper=True),
        }
    )
)


async def to_code(config):
    component = await cg.get_variable(config[CONF_DLMS_COSEM_ID])
    var = await text_sensor.new_text_sensor(config)
    if config[CONF_TYPE] == "CONNECTION":
        cg.add(component.register_connection_sensor(var))
