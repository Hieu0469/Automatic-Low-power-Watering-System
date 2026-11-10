#include "SensorService.h"

#include "AppConfig.h"
#include "ConfigService.h"
#include "PumpController.h"

SensorService::SensorService(ConfigService &config, PumpController &pump)
    : config_(config), pump_(pump), dht_(AppConfig::kDhtPin) {}

bool SensorService::begin() {
    dht_.begin();
    soil_.begin();
    telemetryQueue_ = xQueueCreate(1, sizeof(TelemetrySnapshot));
    if (!telemetryQueue_) {
        Serial.println("[Sensor] Khong tao duoc telemetry queue.");
        return false;
    }
    if (xTaskCreate(taskEntry, "SensorTask", 4096, this, 1, &taskHandle_) != pdPASS) {
        Serial.println("[Sensor] Khong tao duoc SensorTask.");
        return false;
    }
    return true;
}

bool SensorService::pollTelemetry(TelemetrySnapshot &sample) {
    return telemetryQueue_ && xQueueReceive(telemetryQueue_, &sample, 0) == pdTRUE;
}

void SensorService::taskEntry(void *context) {
    static_cast<SensorService *>(context)->run();
}

void SensorService::publishInvalid(const char *reason, int soilRaw) {
    filteredSoilPct_ = NAN;
    wateringPolicy_.reset();
    tankMonitor_.invalidate();
    pump_.request(0);

    TelemetrySnapshot sample;
    sample.soilRaw = soilRaw;
    sample.uptimeMs = millis();
    sample.error = reason;
    sample.noWaterAlert = tankMonitor_.alertActive();
    xQueueOverwrite(telemetryQueue_, &sample);
}

void SensorService::run() {
    TickType_t lastWake = xTaskGetTickCount();
    for (;;) {
        const uint32_t lastPumpMs = pump_.takeLastRunMs();
        // Đọc đất độc lập để vẫn nhận ra lỗi ADC khi DHT22 hỏng.
        const SoilReading soilReading = soil_.read();
        const bool dhtValid = dht_.read();
        if (!soilReading.valid || !dhtValid) {
            Serial.printf("[Sensor] Loi: soilRaw=%d soilValid=%d dhtValid=%d. Dung bom.\n",
                          soilReading.raw, soilReading.valid, dhtValid);
            publishInvalid(!soilReading.valid ? "soil" : "dht", soilReading.raw);
            vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::kSensorIntervalMs));
            continue;
        }

        const float temp = dht_.temperature();
        const float hum = dht_.humidity();
        filteredSoilPct_ = isnan(filteredSoilPct_)
                               ? soilReading.moisturePct
                               : AppConfig::kSoilEmaAlpha * soilReading.moisturePct +
                                     (1.0f - AppConfig::kSoilEmaAlpha) * filteredSoilPct_;

        float waterPct = 0.0f;
        if (!config_.calculateWaterPercent(temp, hum, filteredSoilPct_, waterPct)) {
            publishInvalid("config_busy", soilReading.raw);
            vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::kSensorIntervalMs));
            continue;
        }
        if (!isfinite(waterPct) || waterPct < 0.0f || waterPct > 100.0f) {
            Serial.println("[Sensor] Ket qua luat mo khong hop le. Dung bom.");
            publishInvalid("fuzzy", soilReading.raw);
            vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::kSensorIntervalMs));
            continue;
        }

        const float requestedMs = waterPct * AppConfig::kMaxWaterTimeMs / 100.0f;
        const uint32_t pumpMs = wateringPolicy_.update(requestedMs);
        tankMonitor_.update(filteredSoilPct_, soilReading.moisturePct, lastPumpMs);

        TelemetrySnapshot sample;
        sample.temperatureC = temp;
        sample.airHumidityPct = hum;
        sample.soilMoisturePct = filteredSoilPct_;
        sample.soilRaw = soilReading.raw;
        sample.pumpCommandMs = pumpMs;
        sample.noWaterFailCount = tankMonitor_.failCount();
        sample.noWaterAlert = tankMonitor_.alertActive();
        sample.sensorValid = true;
        sample.uptimeMs = millis();
        xQueueOverwrite(telemetryQueue_, &sample);
        pump_.request(pumpMs);

        Serial.printf("[Sensor] Temp=%.1fC Hum=%.1f%% SoilRaw=%d Soil=%.1f%% "
                      "Request=%.0fms Pump=%lums Fail=%d\n",
                      temp, hum, soilReading.raw, filteredSoilPct_, requestedMs,
                      (unsigned long)pumpMs, sample.noWaterFailCount);
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::kSensorIntervalMs));
    }
}
