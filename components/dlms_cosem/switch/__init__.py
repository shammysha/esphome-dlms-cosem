
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import (
    DEVICE_CLASS_SWITCH,
    ENTITY_CATEGORY_CONFIG,
    CONF_SCAN
)
from .. import (
    DlmsCosem,
    dlms_cosem_ns,
    CONF_DLMS_COSEM_ID,
)

AUTO_LOAD = ["dlms_cosem"]

ICON_SCAN = "mdi:magnify"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_DLMS_COSEM_ID): cv.use_id(DlmsCosem),
        cv.Optional(CONF_SCAN):  switch.switch_schema(
            device_class=DEVICE_CLASS_SWITCH,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_SCAN,
        ),        
    }
)

async def to_code(config):
    hub = await cg.get_variable(config[CONF_DLMS_COSEM_ID])

    if conf := config.get(CONF_SCAN):
        switch = await switch.switch(config[CONF_SCAN])
        cg.add(hub.set_scan_switch(switch))
        
