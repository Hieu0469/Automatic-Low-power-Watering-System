#pragma once

#include <Arduino.h>

/** @file Telemetry.h @brief Các bản tin nội bộ chuyển từ task sang vòng lặp ứng dụng. */

/** @brief Kết quả một chu kỳ đo, dùng để phát JSON telemetry. */
struct TelemetrySnapshot {
    float temperatureC = 0.0f;
    float airHumidityPct = 0.0f;
    float soilMoisturePct = 0.0f;
    int soilRaw = 0;
    uint32_t pumpCommandMs = 0;
    int noWaterFailCount = 0;
    uint32_t uptimeMs = 0;
    bool sensorValid = false;
    bool noWaterAlert = false;
    const char *error = "none"; ///< Chuỗi hằng: none, soil, dht, fuzzy.
};

/** @brief Trạng thái vật lý của chân bơm sau khi PumpController đổi trạng thái. */
struct PumpSnapshot {
    bool pumpOn = false;
    uint32_t wateringCount = 0;
    uint32_t uptimeMs = 0;

    PumpSnapshot() = default;
    PumpSnapshot(bool on, uint32_t count, uint32_t uptime)
        : pumpOn(on), wateringCount(count), uptimeMs(uptime) {}
};
