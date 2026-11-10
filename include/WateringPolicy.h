#pragma once

#include <Arduino.h>

#include "AppConfig.h"

/** @brief Xác nhận nhu cầu tưới và áp dụng ngưỡng bật/tắt khác nhau. */
class WateringPolicy {
  public:
    /** @brief Trả lệnh bơm theo thời gian gợi ý từ luật mờ. */
    uint32_t update(float requestedMs) {
        if (!enabled_) {
            startCount_ = requestedMs >= AppConfig::kMinPumpTimeMs ? startCount_ + 1 : 0;
            if (startCount_ >= AppConfig::kPumpConfirmCycles) {
                enabled_ = true;
                startCount_ = 0;
            }
        } else if (requestedMs <= AppConfig::kPumpStopThresholdMs) {
            enabled_ = false;
            startCount_ = 0;
        }
        return enabled_ ? (uint32_t)constrain(requestedMs, (float)AppConfig::kMinPumpTimeMs,
                                              (float)AppConfig::kMaxWaterTimeMs)
                        : 0;
    }

    /** @brief Hủy xác nhận khi một chu kỳ đo bị lỗi. */
    void reset() {
        enabled_ = false;
        startCount_ = 0;
    }

  private:
    bool enabled_ = false;
    uint8_t startCount_ = 0;
};
