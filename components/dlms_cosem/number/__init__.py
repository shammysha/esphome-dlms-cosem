
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    DEVICE_CLASS_NUMBER,
    ENTITY_CATEGORY_CONFIG,
    CONF_SCAN
)
from .. import (
    DlmsCosem,
    dlms_cosem_ns,
    CONF_DLMS_COSEM_ID,
    CONF_PHYSICAL_DEVICE,
    CONF_LOGICAL_DEVICE,
    CONF_SERVER_ADDRESS,
    CONF_ADDRESS_LENGTH
)

AUTO_LOAD = ["dlms_cosem"]

ICON_SCAN = "mdi:magnify"
ICON_ADDR = "mdi:vector-line"

LogicalDeviceNumber = dlms_cosem_ns.class_("LogicalDeviceNumber", number.Number)
PhysicalDeviceNumber = dlms_cosem_ns.class_("PhysicalDeviceNumber", number.Number)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_DLMS_COSEM_ID): cv.use_id(DlmsCosem),
    cv.Inclusive(CONF_LOGICAL_DEVICE, CONF_SERVER_ADDRESS): number.number_schema(
        LogicalDeviceNumber,
        device_class=DEVICE_CLASS_NUMBER,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_ADDR,    
    ),
    cv.Inclusive(CONF_PHYSICAL_DEVICE, CONF_SERVER_ADDRESS): number.number_schema(
        PhysicalDeviceNumber,
        device_class=DEVICE_CLASS_NUMBER,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_ADDR,    
    ),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_DLMS_COSEM_ID])
    
    if conf := config.get(CONF_LOGICAL_DEVICE):
        n = await number.new_number(conf, min_value=1, max_value=255, step=1)
        cg.add(hub.set_logical_device_number(n))
    if conf := config.get(CONF_PHYSICAL_DEVICE):
        n = await number.new_number(conf, min_value=1, max_value=255, step=1)
        cg.add(hub.set_physical_device_number(n))        
