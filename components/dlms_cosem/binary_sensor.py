
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

CONNECTION_STR = "CONNECTION"

DlmsCosemBinarySensor = dlms_cosem_ns.class_("DlmsCosemBinarySensor", binary_sensor.BinarySensor)

CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(
        DlmsCosemBinarySensor,
    ).extend(
        {
            cv.GenerateID(CONF_DLMS_COSEM_ID): cv.use_id(DlmsCosem),
            cv.Required(CONF_TYPE): cv.Any(CONNECTION_STR),
        }
    )
)


async def to_code(config):
    component = await cg.get_variable(config[CONF_DLMS_COSEM_ID])
    var = await binary_sensor.new_binary_sensor(config)
    if config[CONF_TYPE] == CONNECTION_STR:
        cg.add(component.register_connection_sensor(var))
