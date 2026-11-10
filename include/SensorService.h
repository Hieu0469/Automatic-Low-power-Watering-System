#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "DHT22.h"
#include "SoilSensor.h"
#include "TankMonitor.h"
#include "Telemetry.h"
#include "WateringPolicy.h"

class ConfigService;
class PumpController;

/** @brief Sở hữu tác vụ đọc cảm biến và tạo lệnh tưới/telemetry. */
class SensorService {
  public:
    SensorService(ConfigService &config, PumpController &pump);

    /** @brief Khởi tạo cảm biến, queue và task đo mỗi 10 giây. */
    bool begin();

    /** @brief Lấy snapshot mới nhất để vòng lặp ứng dụng phát qua BLE. */
    bool pollTelemetry(TelemetrySnapshot &sample);

  private:
    static void taskEntry(void *context);
    void run();
    void publishInvalid(const char *reason, int soilRaw);

    ConfigService &config_;
    PumpController &pump_;
    DHT22Driver dht_;
    SoilSensor soil_;
    WateringPolicy wateringPolicy_;
    TankMonitor tankMonitor_;
    QueueHandle_t telemetryQueue_ = nullptr;
    TaskHandle_t taskHandle_ = nullptr;
    float filteredSoilPct_ = NAN;
};
