#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>

#include "FuzzyConfig.h"

/** @brief Kết quả một lệnh thay đổi luật tưới. */
enum class ConfigUpdateResult { Applied, Invalid, Busy };

/**
 * @brief Sở hữu cấu hình luật mờ và đồng bộ truy cập giữa BLE, MQTT, sensor task.
 *
 * Các giao thức chỉ gọi service này, không sửa trực tiếp FuzzyConfig::params.
 */
class ConfigService {
  public:
    /** @brief Tạo mutex và nạp cấu hình từ NVS. */
    bool begin();

    /** @brief Tính mức tưới; trả false nếu đang bận cấu hình. */
    bool calculateWaterPercent(float temp, float hum, float soil, float &result);

    /** @brief Áp dụng lệnh MF/RL/RESET của BLE. */
    ConfigUpdateResult applyCommand(const String &command);

    /** @brief Áp dụng toàn bộ cấu hình JSON của MQTT trong một lần lưu NVS. */
    ConfigUpdateResult applyJson(JsonVariantConst json);

    /** @brief Trả JSON hiện tại; false nếu không lấy được mutex. */
    bool toJSON(String &json);

  private:
    FuzzyConfig config_;
    SemaphoreHandle_t mutex_ = nullptr;
};
