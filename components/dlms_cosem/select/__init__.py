
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import (
    DEVICE_CLASS_SELECT,
    ENTITY_CATEGORY_CONFIG,
    CONF_SCAN
)
from .. import (
    DlmsCosem,
    dlms_cosem_ns,
    CONF_DLMS_COSEM_ID,
    CONF_SERVER_ADRESS,
    CONF_ADDRESS_LENGTH
)

AUTO_LOAD = ["dlms_cosem"]

ICON_SCAN = "mdi:magnify"
ICON_ADDR = "mdi:vector-line"

AddressLengthSelect = dlms_cosem_ns.class_("AddressLengthSelect", select.Select)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_DLMS_COSEM_ID): cv.use_id(DlmsCosem),
    cv.Inclusive(CONF_ADDRESS_LENGTH, CONF_SERVER_ADDRESS): select.select_schema(
        AddressLengthSelect,
        device_class=DEVICE_CLASS_SELECT,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_ADDR,    
    )
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_DLMS_COSEM_ID])
    
    if conf := config.get(CONF_ADDRESS_LENGTH):
        n = await select.new_select(conf, options=["1","2","4"])
        cg.add(hub.set_address_length_select(n))
