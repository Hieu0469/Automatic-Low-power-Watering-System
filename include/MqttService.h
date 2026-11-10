#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "Telemetry.h"

class ConfigService;

/**
 * @brief Kết nối Wi-Fi/MQTT và ánh xạ các bản tin MQTT sang cấu hình thiết bị.
 *
 * Gọi begin() từ setup và các hàm còn lại từ Arduino loop(), không gọi từ task.
 * SensorService và PumpController chuyển dữ liệu cho WateringApp qua queue.
 */
class MqttService {
  public:
    explicit MqttService(ConfigService &config);

    /** @brief Chuẩn bị topic, callback và Wi-Fi; không chặn chờ kết nối. */
    void begin();

    /** @brief Duy trì Wi-Fi/MQTT, thử kết nối lại theo chu kỳ. */
    void tick();

    /** @return true nếu broker đang kết nối. */
    bool connected();

    /** @return true đúng một lần sau mỗi lần kết nối MQTT thành công. */
    bool takeJustConnected();

    /** @brief Phát kết quả đo mới nhất, retained để dashboard mở muộn vẫn thấy. */
    bool publishTelemetry(const TelemetrySnapshot &sample);

    /** @brief Phát trạng thái bơm thực tế và số lần tưới. */
    bool publishPumpState(const PumpSnapshot &state);

    /** @brief Phát toàn bộ cấu hình luật mờ hiện tại. */
    bool publishConfig();

  private:
    static MqttService *active_;
    static void onMessageStatic(char *topic, uint8_t *payload, unsigned int length);

    void onMessage(const char *topic, const uint8_t *payload, unsigned int length);
    void publishAck(const char *requestId, bool ok, const char *reason);
    bool publish(const String &topic, const char *payload, bool retained);

    ConfigService &config_;
    WiFiClient wifiClient_;
    PubSubClient mqtt_;
    String baseTopic_;
    String statusTopic_;
    String telemetryTopic_;
    String pumpTopic_;
    String commandTopic_;
    String configStateTopic_;
    String ackTopic_;
    bool enabled_ = false;
    bool justConnected_ = false;
    uint32_t lastWifiAttemptMs_ = 0;
    uint32_t lastMqttAttemptMs_ = 0;
};
