#pragma once

#include <Arduino.h>

#include "ConfigService.h"
#include "PumpController.h"
#include "SensorService.h"
#include "Telemetry.h"

/** @brief Điều phối các module và chuyển snapshot từ task sang BLE. */
class WateringApp {
  public:
    WateringApp();

    /** @brief Khởi động các module theo thứ tự phụ thuộc. */
    void begin();

    /** @brief Xử lý trạng thái task và cảnh báo; gọi từ Arduino loop(). */
    void tick();

  private:
    void updateAlert(const TelemetrySnapshot &sample);
    void serviceBle();

    ConfigService config_;
    PumpController pump_;
    SensorService sensors_;
    bool ready_ = false;
    bool alertKnown_ = false;
    bool tankAlertActive_ = false;
    uint32_t lastAlertRepeatMs_ = 0;
};
