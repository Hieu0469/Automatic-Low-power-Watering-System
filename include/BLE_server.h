#pragma once
#include <Arduino.h>
class ConfigService;

/** @file BLE_server.h @brief Giao diện BLE cũ, chạy song song với MQTT. */

/** @brief Khởi tạo BLE service và advertising. */
void ble_server_init(ConfigService &config);
/** @brief Notify bản tin cảm biến dạng CSV cho dashboard BLE. */
void ble_send_sensor_data(float temp, float hum, float soil, int waterMs, int failCount);
/** @brief Notify JSON cấu hình, tự chia gói theo MTU BLE. */
void ble_send_config(const String &json);
/** @brief Notify thông báo cảnh báo. */
void ble_send_alert_msg(const String &msg);
/** @return true khi có BLE client đang kết nối. */
bool ble_is_connected();
/** @return true khi client yêu cầu cấu hình hiện tại. */
bool ble_config_requested();
/** @brief Xóa cờ yêu cầu cấu hình. */
void ble_clear_config_request();
/** @brief true nếu BLE vừa thay đổi luật mờ và MQTT cần phát lại config. */
bool ble_config_changed();
/** @brief Xóa cờ thay đổi sau khi MQTT đã phát cấu hình. */
void ble_clear_config_changed();
