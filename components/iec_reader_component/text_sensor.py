import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY

from . import IECReaderComponent, CONF_IEC_READER_ID

PLATFORM_SCHEMA = text_sensor.TEXT_SENSOR_PLATFORM_SCHEMA.extend({
    cv.GenerateID(CONF_IEC_READER_ID): cv.use_id(IECReaderComponent),
})

def to_code(config):
    pass
