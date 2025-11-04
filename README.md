⚡ ESPHome IEC 62056-21 Meter Reader (Custom Component)
Dự án này cung cấp một Custom Component cho ESPHome, cho phép ESP32 đọc dữ liệu công tơ điện tử theo chuẩn IEC 62056-21 (Mode C) thông qua giao tiếp Serial (UART) và tích hợp mượt mà với Home Assistant qua ESPHome API.

⚙️ Cấu trúc Component
Component được tổ chức trong thư mục iec_reader_component và chứa logic C++ để xử lý:

Thiết lập phiên: Khởi tạo giao tiếp (300 Baud) và chuyển sang tốc độ dữ liệu (4800 Baud).

Giao thức: Xử lý các bản tin /?!\r\n, <ACK>040\r\n, và <SOH>B0<ETX>u.

Phân tích OBIS: Trích xuất các giá trị số (Voltage, Current, Energy) và giá trị chuỗi (Meter ID, Time).

Xuất dữ liệu: Cập nhật dữ liệu dưới dạng các thực thể sensor (số) và text_sensor (chuỗi) của ESPHome.

🛠️ Yêu cầu Phần cứng
Vi điều khiển: ESP32 (Bo mạch được khai báo là m5stack_atom trong YAML mẫu).

Giao tiếp: Mạch chuyển đổi từ TTL/GPIO sang RS-232 hoặc Đầu đọc quang/hồng ngoại tuân thủ chuẩn IEC.

Chân UART (Mặc định):

RX: GPIO 22

TX: GPIO 19

UART: UART2

🚀 Hướng dẫn Cài đặt & Triển khai
Để sử dụng component này, bạn cần thêm nó vào file cấu hình YAML của dự án ESPHome (ví dụ: my_project.yaml).

Bước 1: Khai báo Custom Component
Thêm đoạn sau vào file cấu hình .yaml của bạn. Đảm bảo thay thế URL và ref bằng thông tin Git của bạn.

YAML

external_components:
  - source: 
      type: git
      # *** THAY THẾ DÒNG NÀY BẰNG URL GITHUB CỦA BẠN ***
      url: https://github.com/user_name/repo_name.git 
      ref: main
    components: [iec_reader_component] 
Bước 2: Khai báo Thực thể Cảm biến (Sensors)
Sử dụng lambda để khởi tạo component và khai báo tất cả các thực thể sensor:

YAML

sensor:
  - platform: custom
    # Khởi tạo component và đăng ký các sensor số
    lambda: |-
      auto iec_sensor = new IECReaderComponent();
      App.register_component(iec_sensor);
      return {iec_sensor->voltage_sensor, iec_sensor->current_sensor, iec_sensor->energy_sensor};

    sensors:
      - name: "IEC Voltage"
        unit_of_measurement: "V"
        accuracy_decimals: 2
        id: iec_voltage_sensor
      - name: "IEC Current"
        unit_of_measurement: "A"
        accuracy_decimals: 2
        id: iec_current_sensor
      - name: "IEC Energy Total"
        unit_of_measurement: "kWh"
        accuracy_decimals: 2
        id: iec_energy_sensor

text_sensor:
  - platform: custom
    # Lưu ý: Lambda cho text_sensor phải là instance MỚI của component
    lambda: |-
      auto iec_sensor = new IECReaderComponent();
      App.register_component(iec_sensor);
      return {iec_sensor->meter_id_sensor, iec_sensor->meter_time_sensor};

    text_sensors:
      - name: "IEC Meter ID"
        id: iec_meter_id_sensor
      - name: "IEC Meter Time"
        id: iec_meter_time_sensor
Dự án được xây dựng dựa trên các tiêu chuẩn IEC 62056-21 Mode C.