import re
from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_AUTH,
    CONF_BAUD_RATE,
    CONF_RECEIVE_TIMEOUT,
    CONF_UPDATE_INTERVAL,
    CONF_FLOW_CONTROL_PIN,
    CONF_PASSWORD,
    CONF_DEVICE_CLASS,
    CONF_ENTITY_CATEGORY,
    CONF_DISABLED_BY_DEFAULT,
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC
)

CODEOWNERS = ["@latonita"]

AUTO_LOAD = ["binary_sensor"]

DEPENDENCIES = ["uart"]

DEFAULTS_MAX_SENSOR_INDEX = 12
DEFAULTS_BAUD_RATE_HANDSHAKE = 9600
DEFAULTS_BAUD_RATE_SESSION = 9600
DEFAULTS_RECEIVE_TIMEOUT = "500ms"
DEFAULTS_DELAY_BETWEEN_REQUESTS = "50ms"
DEFAULTS_UPDATE_INTERVAL = "30s"

CONF_DLMS_COSEM_ID = "dlms_cosem_id"
CONF_OBIS_CODE = "obis_code"
CONF_CLIENT_ADDRESS = "client_address"
CONF_SERVER_ADDRESS = "server_address"
CONF_LOGICAL_DEVICE = "logical_device"
CONF_PHYSICAL_DEVICE = "physical_device"
CONF_ADDRESS_LENGTH = "address_length"
CONF_DELAY_BETWEEN_REQUESTS = "delay_between_requests"
CONF_DONT_PUBLISH = "dont_publish"
CONF_CLASS = "class"

CONF_INDICATOR = "indicator"
CONF_REBOOT_AFTER_FAILURE = "reboot_after_failure"

CONF_BAUD_RATE_HANDSHAKE = "baud_rate_handshake"

dlms_cosem_ns = cg.esphome_ns.namespace("dlms_cosem")
DlmsCosem = dlms_cosem_ns.class_(
    "DlmsCosemComponent", cg.Component, uart.UARTDevice
)

BAUD_RATES = [300, 600, 1200, 2400, 4800, 9600, 19200]
ADDRESS_LENGTH_ENUM = [1, 2, 4]

def obis_code(value):
    value = cv.string(value)
#    match = re.match(r"^\d{1,3}-\d{1,3}:\d{1,3}\.\d{1,3}\.\d{1,3}$", value)
    match = re.match(r"^\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3}\.\d{1,3}\.\d{1,3}$", value)
    if match is None:
        raise cv.Invalid(f"{value} is not a valid OBIS code")
    return value


def validate_meter_address(value):
    if len(value) > 15:
        raise cv.Invalid("Meter address length must be no longer than 15 characters")
    return value


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DlmsCosem),
            cv.Optional(CONF_CLIENT_ADDRESS, default=16): cv.positive_int,
            cv.Optional(CONF_SERVER_ADDRESS): cv.positive_int,
            cv.Optional(CONF_LOGICAL_DEVICE): cv.positive_int,
            cv.Optional(CONF_PHYSICAL_DEVICE): cv.positive_int,
            cv.Optional(CONF_ADDRESS_LENGTH): cv.one_of(1, 2, 4),
            cv.Optional(CONF_AUTH, default=False): cv.boolean,
            cv.Optional(CONF_PASSWORD, default=""): cv.string,
            cv.Optional(CONF_FLOW_CONTROL_PIN): pins.gpio_output_pin_schema,
            cv.Optional(
                CONF_BAUD_RATE_HANDSHAKE, default=DEFAULTS_BAUD_RATE_HANDSHAKE
            ): cv.one_of(*BAUD_RATES),
            cv.Optional(CONF_BAUD_RATE, default=DEFAULTS_BAUD_RATE_SESSION): cv.one_of(
                *BAUD_RATES
            ),
            cv.Optional(
                CONF_RECEIVE_TIMEOUT, default=DEFAULTS_RECEIVE_TIMEOUT
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_DELAY_BETWEEN_REQUESTS, default=DEFAULTS_DELAY_BETWEEN_REQUESTS
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_UPDATE_INTERVAL, default=DEFAULTS_UPDATE_INTERVAL
            ): cv.update_interval,
            cv.Optional(CONF_REBOOT_AFTER_FAILURE, default=0): cv.int_range(
                min=0, max=100
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA),
    cv.has_none_or_all_keys(CONF_LOGICAL_DEVICE, CONF_PHYSICAL_DEVICE, CONF_ADDRESS_LENGTH),
    cv.has_exactly_one_key(CONF_SERVER_ADDRESS, CONF_PHYSICAL_DEVICE)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if flow_control_pin := config.get(CONF_FLOW_CONTROL_PIN):
        pin = await cg.gpio_pin_expression(flow_control_pin)
        cg.add(var.set_flow_control_pin(pin))

    if indicator_config := config.get(CONF_INDICATOR):
        sens = cg.new_Pvariable(indicator_config[CONF_ID])
        await binary_sensor.register_binary_sensor(sens, indicator_config)
        cg.add(var.set_indicator(sens))

    if config.get(CONF_SERVER_ADDRESS):
        cg.add(var.set_server_address(config[CONF_SERVER_ADDRESS]))    

    if config.get(CONF_LOGICAL_DEVICE):
        cg.add(var.update_server_address(config[CONF_LOGICAL_DEVICE], config[CONF_PHYSICAL_DEVICE], config[CONF_ADDRESS_LENGTH]))    
        
    cg.add(var.set_client_address(config[CONF_CLIENT_ADDRESS]))
    cg.add(var.set_auth_required(config[CONF_AUTH]))
    cg.add(var.set_password(config[CONF_PASSWORD]))
    cg.add(var.set_baud_rates(config[CONF_BAUD_RATE_HANDSHAKE], config[CONF_BAUD_RATE]))
    cg.add(var.set_receive_timeout_ms(config[CONF_RECEIVE_TIMEOUT]))
    cg.add(var.set_delay_between_requests_ms(config[CONF_DELAY_BETWEEN_REQUESTS]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    cg.add(var.set_reboot_after_failure(config[CONF_REBOOT_AFTER_FAILURE]))

    cg.add(
        var.set_connection_sensor(await binary_sensor.new_binary_sensor(
            {
                CONF_ID: str(config.get(CONF_ID)) + "_connection",
                CONF_NAME: (config.get(CONF_NAME) or str(config.get(CONF_ID))).replace("_", "-") + "-connection",
                CONF_DEVICE_CLASS: DEVICE_CLASS_CONNECTIVITY,
                CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
                CONF_DISABLED_BY_DEFAULT: False
            }            
        ))
    )
    
#    cg.add_library("GuruxDLMS", None, "https://github.com/latonita/GuruxDLMS.c.platformio")
    cg.add_library("GuruxDLMS", None, "https://github.com/viric/GuruxDLMS.c#platformio")
