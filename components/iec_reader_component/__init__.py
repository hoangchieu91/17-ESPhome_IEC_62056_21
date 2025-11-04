import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

# Khai báo không gian tên C++ và tên class
# iec_reader_component là tên namespace, IECReaderComponent là tên class C++
CONF_IEC_READER_ID = "iec_reader_component"
iec_reader_component_ns = cg.esphome_ns.namespace(CONF_IEC_READER_ID)
IECReaderComponent = iec_reader_component_ns.class_("IECReaderComponent", cg.Component)

# Không cần thêm cấu hình YAML phức tạp cho component, chỉ cần ID
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(IECReaderComponent),
}).extend(cv.COMPONENT_SCHEMA)

# Hàm đăng ký (tương đương với setup())
def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
