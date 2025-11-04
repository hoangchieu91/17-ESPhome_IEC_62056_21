#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h" // Thêm thư viện text_sensor
#include "driver/uart.h"
#include <string>
#include <algorithm>
#include <cmath>

// Ghi đè lên chân UART (UART2)
#define UART_NUM UART_NUM_2 
#define RX_PIN 22
#define TX_PIN 19

namespace esphome {
namespace iec_reader_component {

static const char *const TAG = "iec.reader";

class IECReaderComponent : public Component {
 public:
  // Khai báo các Sensor giá trị số (Voltage, Current, Energy)
  sensor::Sensor *voltage_sensor = new sensor::Sensor();
  sensor::Sensor *current_sensor = new sensor::Sensor();
  sensor::Sensor *energy_sensor = new sensor::Sensor();

  // THÊM: Khai báo các Text Sensor cho ID và Time
  text_sensor::TextSensor *meter_id_sensor = new text_sensor::TextSensor();
  text_sensor::TextSensor *meter_time_sensor = new text_sensor::TextSensor();


  void setup() override {
    // 1. Cấu hình UART ban đầu (300 baud, 7E1)
    uart_config_t uart_config = {
        .baud_rate = 300,
        .data_bits = UART_DATA_7_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Cài đặt driver UART
    uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "UART2 driver installed on TX=%d, RX=%d", RX_PIN, TX_PIN);
  }

  void loop() override {
    // Sử dụng loop của ESPHome
    static unsigned long last_scan_time = 0;
    const unsigned long SCAN_INTERVAL_MS = 30000; // 30 giây

    if (millis() - last_scan_time >= SCAN_INTERVAL_MS) {
      last_scan_time = millis();
      ESP_LOGI(TAG, "--- STARTING IEC SCAN CYCLE ---");
      scan_iec_meter();
    }
  }

  // --- LOGIC PHÂN TÍCH VÀ GIAO TIẾP ---

  std::string extract_obis_value(const std::string& data_block, const char* obis_code) {
    std::string search_str = std::string(obis_code) + "(";
    size_t start_idx = data_block.find(search_str);

    if (start_idx != std::string::npos) {
        size_t value_start = start_idx + search_str.length();
        size_t value_end = data_block.find(')', value_start);

        if (value_end != std::string::npos) {
            std::string value_unit_str = data_block.substr(value_start, value_end - value_start);
            size_t star_idx = value_unit_str.find('*');
            if (star_idx != std::string::npos) {
                // Trả về giá trị trước dấu '*'
                value_unit_str = value_unit_str.substr(0, star_idx);
            }
            // Loại bỏ khoảng trắng và ký tự điều khiển
            value_unit_str.erase(std::remove_if(value_unit_str.begin(), value_unit_str.end(), ::isspace), value_unit_str.end());
            return value_unit_str;
        }
    }
    return "";
  }

  void parse_and_publish(const std::string& data_block) {
    // Trích xuất giá trị số
    float voltage = strtof(extract_obis_value(data_block, "1.0.32.7.0").c_str(), NULL);
    float current = strtof(extract_obis_value(data_block, "1.0.31.7.0").c_str(), NULL);
    float energy = strtof(extract_obis_value(data_block, "1.0.1.8.0").c_str(), NULL);
    
    // THÊM: Trích xuất giá trị chuỗi (ID và Time)
    std::string meter_id = extract_obis_value(data_block, "0.0.C.1.0"); // ID đồng hồ
    std::string meter_time = extract_obis_value(data_block, "0.0.0.9.1"); // Giờ (HHMMSS)
    std::string meter_date = extract_obis_value(data_block, "0.0.0.9.2"); // Ngày (YYMMDD)

    // Cập nhật các sensor số
    if (!std::isnan(voltage)) voltage_sensor->publish_state(voltage);
    if (!std::isnan(current)) current_sensor->publish_state(current);
    if (!std::isnan(energy)) energy_sensor->publish_state(energy);
    
    // Cập nhật các Text Sensor
    if (!meter_id.empty()) meter_id_sensor->publish_state(meter_id);
    if (!meter_time.empty() && !meter_date.empty()) {
        std::string full_time = meter_date + " " + meter_time;
        meter_time_sensor->publish_state(full_time);
    }

    ESP_LOGI(TAG, "Data published: ID=%s, V=%.2f, Time=%s", meter_id.c_str(), voltage, meter_time.c_str());
  }


  // Gửi lệnh và đọc phản hồi (Giữ nguyên)
  std::string send_and_read(const char *cmd, int baud, uart_parity_t parity_mode, uint32_t timeout_ms) {
    // Chỉ set baud rate và các thông số cần thiết
    uart_set_baudrate(UART_NUM, baud); 
    uart_set_parity(UART_NUM, parity_mode);
    
    // Xóa buffer trước khi gửi
    uart_flush(UART_NUM); 

    uart_write_bytes(UART_NUM, cmd, strlen(cmd));
    uart_wait_tx_done(UART_NUM, 100 / portTICK_PERIOD_MS);

    std::string buffer;
    char data[128];
    int len;

    // Đọc phản hồi
    len = uart_read_bytes(UART_NUM, (uint8_t*)data, sizeof(data) - 1, timeout_ms / portTICK_PERIOD_MS);
    if (len > 0) {
        data[len] = '\0';
        buffer = data;
        // Loại bỏ ký tự điều khiển (CR, LF)
        buffer.erase(std::remove(buffer.begin(), buffer.end(), '\r'), buffer.end());
        buffer.erase(std::remove(buffer.begin(), buffer.end(), '\n'), buffer.end());
    }
    return buffer;
  }

  std::string read_data_block(int baud) {
    // Thay đổi tốc độ baud và các thông số (7E1)
    uart_set_baudrate(UART_NUM, baud); 
    uart_set_word_length(UART_NUM, UART_DATA_7_BITS);
    uart_set_parity(UART_NUM, UART_PARITY_EVEN);
    uart_set_stop_bits(UART_NUM, UART_STOP_BITS_1);
    
    vTaskDelay(200 / portTICK_PERIOD_MS); // Cho phép công tơ bắt đầu truyền

    std::string buffer;
    char data[64];
    int len;
    uint32_t start_time = millis();

    // Đọc cho đến khi Timeout hoặc nhận đủ dữ liệu
    while (millis() - start_time < 5000) { // Timeout 5 giây
        len = uart_read_bytes(UART_NUM, (uint8_t*)data, sizeof(data) - 1, 50 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';
            buffer += data;
            // Kiểm tra ký tự kết thúc: <ETX> (0x03)
            if (buffer.find('\x03') != std::string::npos) {
                break;
            }
        }
    }
    return buffer;
  }
  
  // --- CHU KỲ QUÉT CHÍNH ---
  void scan_iec_meter() {
    const char *IDENTIFY_REQUEST = "/?!\r\n";
    const char *READOUT_MODE_COMMAND = "\x06" "040\r\n"; // 4800 baud
    const char *CLOSE_SESSION_COMMAND = "\x01" "B0" "\x03" "\x75"; 

    // 1. PHASE 1: NHẬN DẠNG (300 baud, 7E1)
    std::string identify_resp = send_and_read(IDENTIFY_REQUEST, 300, UART_PARITY_EVEN, 1500);

    if (identify_resp.empty()) {
        ESP_LOGE(TAG, "Failed to get Identify Response. Is meter connected?");
        voltage_sensor->publish_state(NAN); // Đặt trạng thái không khả dụng
        return;
    }
    ESP_LOGI(TAG, "Identify Response: %s", identify_resp.c_str());

    // 2. PHASE 2: CHUYỂN TỐC ĐỘ (300 baud)
    uart_write_bytes(UART_NUM, READOUT_MODE_COMMAND, strlen(READOUT_MODE_COMMAND));
    uart_wait_tx_done(UART_NUM, 100 / portTICK_PERIOD_MS);

    // 3. PHASE 3: ĐỌC DỮ LIỆU (4800 baud, 7E1)
    std::string data_block = read_data_block(4800);

    if (data_block.empty() || data_block.find('(') == std::string::npos) {
        ESP_LOGE(TAG, "Failed to read valid data block at 4800 baud.");
        voltage_sensor->publish_state(NAN);
    } else {
        parse_and_publish(data_block);
    }
    
    // 4. PHASE 4: ĐÓNG PHIÊN VÀ RESET UART
    uart_write_bytes(UART_NUM, CLOSE_SESSION_COMMAND, strlen(CLOSE_SESSION_COMMAND));
    uart_wait_tx_done(UART_NUM, 100 / portTICK_PERIOD_MS);
    
    // Đảm bảo trở về 300 baud cho chu kỳ tiếp theo
    uart_set_baudrate(UART_NUM, 300); 
    vTaskDelay(100 / portTICK_PERIOD_MS); // Khoảng nghỉ cần thiết để công tơ reset
  }

};

} // namespace iec_reader_component
} // namespace esphome
