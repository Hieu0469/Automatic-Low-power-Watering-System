#pragma once

/**
 * @file MqttSettings.example.h
 * @brief Cấu hình mạng mẫu. Sao chép thành MqttSettings.h rồi điền thông số thật.
 *
 * MqttSettings.h được .gitignore loại trừ để không đưa mật khẩu lên Git.
 * Nếu chưa tạo file riêng, firmware vẫn build nhưng MQTT sẽ không khởi động.
 */
namespace MqttSettings {
constexpr char kWifiSsid[] = "";
constexpr char kWifiPassword[] = "";
constexpr char kBrokerHost[] = ""; ///< IP/tên miền broker, không gồm mqtt://.
constexpr unsigned short kBrokerPort = 1883;
constexpr char kBrokerUser[] = ""; ///< Để trống nếu broker không yêu cầu.
constexpr char kBrokerPassword[] = "";
constexpr char kDeviceId[] = "esp32-01"; ///< Chỉ dùng chữ, số, dấu _ hoặc -.
} // namespace MqttSettings
