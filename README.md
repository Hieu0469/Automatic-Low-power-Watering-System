# Automatic Low-power Watering System

Hệ thống tưới cây tự động dùng **ESP32**, cảm biến DHT22 và cảm biến độ ẩm đất. Firmware đọc môi trường theo chu kỳ, tính nhu cầu tưới bằng luật mờ, điều khiển bơm và gửi dữ liệu đến dashboard qua **Bluetooth Low Energy (BLE)**.

> Nhánh `main` chứa firmware và dashboard BLE. Phần giao tiếp MQTT giữa ESP32 và dashboard được phát triển riêng trên nhánh [`feature/mqtt`](https://github.com/Hieu0469/Automatic-Low-power-Watering-System/tree/feature/mqtt); xem [hướng dẫn MQTT](https://github.com/Hieu0469/Automatic-Low-power-Watering-System/blob/feature/mqtt/docs/MQTT.md) khi làm việc trên nhánh đó.

## Chức năng hiện có trên `main`

- Đọc nhiệt độ và độ ẩm không khí từ DHT22 mỗi **10 giây**.
- Đọc ADC độ ẩm đất trên GPIO34: lấy **trung vị 16 mẫu**, quy đổi sang phần trăm và lọc EMA để giảm dao động giữa các chu kỳ.
- Tính mức tưới từ nhiệt độ, độ ẩm không khí và độ ẩm đất bằng 9 hàm membership cùng 27 luật mờ. Cấu hình luật được lưu trong NVS và có thể chỉnh từ dashboard BLE.
- Xác nhận nhu cầu tưới qua hai chu kỳ và dùng ngưỡng bật/tắt khác nhau để tránh bơm đóng ngắt liên tục. Một lần bơm tối đa **1.500 ms** theo cấu hình hiện tại.
- Dừng yêu cầu tưới khi dữ liệu cảm biến hoặc kết quả tính toán không hợp lệ. Cảnh báo **có thể hết nước** khi đất còn khô sau các lần bơm đã thực sự chạy; đây là suy luận từ độ ẩm đất, không phải phép đo mực nước trực tiếp.

## Phần cứng và chân kết nối

| Thiết bị / tín hiệu | Chân ESP32 | Ghi chú |
| --- | --- | --- |
| DHT22, chân dữ liệu | GPIO15 | Đọc nhiệt độ và độ ẩm không khí. |
| Cảm biến độ ẩm đất, A0 | GPIO34 | Đầu vào ADC dùng để tính phần trăm độ ẩm. |
| Cảm biến độ ẩm đất, D0 | GPIO5 | Được khai báo làm đầu vào; thuật toán tưới hiện dùng A0. |
| Tín hiệu điều khiển bơm | GPIO4 | Mức `HIGH` bật bơm, `LOW` tắt bơm. Nối qua mạch công suất/relay phù hợp, không cấp bơm trực tiếp từ GPIO. |

Các ngưỡng ADC **ướt = 1200** và **khô = 2800** trong [`include/SoilSensor.h`](include/SoilSensor.h) chỉ là giá trị ban đầu. Cần đo giá trị thực tế của cảm biến khi khô và khi ướt rồi hiệu chuẩn trước khi dựa vào phần trăm hiển thị hoặc để bơm chạy tự động.

## Chạy nhanh

Yêu cầu: ESP32 Dev Module, [PlatformIO](https://platformio.org/) và Python 3 nếu muốn mở dashboard bằng máy chủ HTTP cục bộ.

1. Kết nối phần cứng theo bảng trên và mở thư mục dự án trong VS Code/PlatformIO.
2. Biên dịch và nạp firmware:

   ```powershell
   pio run -e esp32dev
   pio run -e esp32dev -t upload
   ```

3. Mở Serial Monitor ở **9600 baud** để xem các log `[Sensor]`, `[Pump]`, `[Tank]`, `[BLE]`:

   ```powershell
   pio device monitor -b 9600
   ```

4. Tại thư mục gốc dự án, chạy dashboard:

   ```powershell
   python -m http.server 8000
   ```

   Mở `http://localhost:8000` bằng trình duyệt có hỗ trợ Web Bluetooth, bật Bluetooth trên máy và bấm **Kết nối BLE**. Thiết bị quảng bá với tên `ESP32_TuoiTieu`. Dashboard hiển thị số đo, cảnh báo và cho phép chỉnh luật tưới.

## Cấu trúc mã nguồn

| Vị trí | Vai trò |
| --- | --- |
| [`src/main.cpp`](src/main.cpp) | Điểm vào Arduino: gọi `WateringApp::begin()` và `WateringApp::tick()`. |
| [`src/SensorService.cpp`](src/SensorService.cpp) | Đọc cảm biến, lọc độ ẩm đất, tính nhu cầu tưới và tạo bản tin nội bộ. |
| [`src/PumpController.cpp`](src/PumpController.cpp) | Nhận lệnh từ queue, điều khiển GPIO4 trong task riêng. |
| [`src/TankMonitor.cpp`](src/TankMonitor.cpp) | Theo dõi số lần tưới không làm độ ẩm đất tăng để phát cảnh báo. |
| [`src/ConfigService.cpp`](src/ConfigService.cpp) | Đồng bộ truy cập cấu hình luật mờ giữa BLE và task cảm biến. |
| [`src/BLE_server.cpp`](src/BLE_server.cpp) | Giao tiếp BLE với dashboard. |
| [`index.html`](index.html) | Dashboard chạy trong trình duyệt. |
| [`docs/STRUCTURE.md`](docs/STRUCTURE.md) | Sơ đồ module, luồng dữ liệu và điểm bắt đầu khi debug. |

Các chân GPIO, chu kỳ đọc và ngưỡng tưới/cảnh báo nằm trong [`include/AppConfig.h`](include/AppConfig.h). Giá trị ADC dùng để quy đổi phần trăm nằm trong [`include/SoilSensor.h`](include/SoilSensor.h).

## Trạng thái kiểm tra

Firmware trên `main` đã biên dịch thành công cho môi trường PlatformIO `esp32dev`. Việc đấu dây, hiệu chuẩn và kiểm tra bơm/cảm biến trên phần cứng thật cần thực hiện với bộ linh kiện của dự án.
