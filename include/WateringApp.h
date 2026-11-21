#pragma once

#include <Arduino.h>

#include "ConfigService.h"
#include "MqttService.h"
#include "PumpController.h"
#include "SensorService.h"
#include "Telemetry.h"

/** @brief Điều phối các module và chuyển snapshot từ task sang BLE/MQTT. */
class WateringApp {
  public:
    WateringApp();

    /** @brief Khởi động các module theo thứ tự phụ thuộc. */
    void begin();

    /** @brief Xử lý mạng, trạng thái task và cảnh báo; gọi từ Arduino loop(). */
    void tick();

  private:
    void updateAlert(const TelemetrySnapshot &sample);
    void serviceBle();

    ConfigService config_;
    PumpController pump_;
    SensorService sensors_;
    MqttService mqtt_;
    TelemetrySnapshot latestTelemetry_;
    PumpSnapshot latestPump_;
    bool ready_ = false;
    bool hasTelemetry_ = false;
    bool hasPump_ = false;
    bool telemetryPending_ = false;
    bool pumpPending_ = false;
    bool alertKnown_ = false;
    bool tankAlertActive_ = false;
    uint32_t lastAlertRepeatMs_ = 0;
};
