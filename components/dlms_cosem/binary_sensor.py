
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import (
    DlmsCosem,
    dlms_cosem_ns,
    CONF_DLMS_COSEM_ID,
)

AUTO_LOAD = ["dlms_cosem"]

CONF_TRANSMISSION = "transmission"
ICON_CONNECT = "mdi:swap-horizontal"

DlmsCosemBinarySensor = dlms_cosem_ns.class_("DlmsCosemBinarySensor", binary_sensor.BinarySensor)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DLMS_COSEM_ID): cv.use_id(DlmsCosem),
        cv.Optional(CONF_TRANSMISSION):  binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_CONNECTIVITY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon=ICON_CONNECT,
        ),
    }
)

async def to_code(config):
    hub = await cg.get_variable(config[CONF_DLMS_COSEM_ID])

    if conf := config.get(CONF_TRANSMISSION):
        sensor = await binary_sensor.new_binary_sensor(config[CONF_TRANSMISSION])
        cg.add(hub.set_transmission_binary_sensor(sensor))
