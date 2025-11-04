import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY

# Khai báo component chính (từ __init__.py)
from . import IECReaderComponent, CONF_IEC_READER_ID

PLATFORM_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_EMPTY,
    icon=ICON_EMPTY,
    accuracy_decimals=2,
).extend({
    cv.GenerateID(CONF_IEC_READER_ID): cv.use_id(IECReaderComponent),
})

# Vì bạn đã dùng platform: custom trong YAML, phần này không cần thiết,
# nhưng nó là cấu trúc chuẩn nếu bạn muốn dùng platform: iec_reader_component
def to_code(config):
    pass 
