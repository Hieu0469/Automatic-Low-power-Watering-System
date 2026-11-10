#include "PumpController.h"

#include "AppConfig.h"

bool PumpController::begin() {
    pinMode(AppConfig::kPumpPin, OUTPUT);
    digitalWrite(AppConfig::kPumpPin, LOW);

    requestQueue_ = xQueueCreate(1, sizeof(uint32_t));
    resultQueue_ = xQueueCreate(1, sizeof(uint32_t));
    stateQueue_ = xQueueCreate(8, sizeof(PumpSnapshot));
    if (!requestQueue_ || !resultQueue_ || !stateQueue_) {
        Serial.println("[Pump] Khong tao duoc queue.");
        return false;
    }
    enqueueState(PumpSnapshot{false, 0, millis()});
    if (xTaskCreate(taskEntry, "WateringTask", 4096, this, 2, &taskHandle_) != pdPASS) {
        Serial.println("[Pump] Khong tao duoc WateringTask.");
        return false;
    }
    return true;
}

void PumpController::request(uint32_t durationMs) {
    if (requestQueue_)
        xQueueOverwrite(requestQueue_, &durationMs);
}

uint32_t PumpController::takeLastRunMs() {
    uint32_t lastRunMs = 0;
    if (resultQueue_)
        xQueueReceive(resultQueue_, &lastRunMs, 0);
    return lastRunMs;
}

bool PumpController::pollState(PumpSnapshot &state) {
    return stateQueue_ && xQueueReceive(stateQueue_, &state, 0) == pdTRUE;
}

void PumpController::enqueueState(const PumpSnapshot &state) {
    if (xQueueSendToBack(stateQueue_, &state, 0) == pdTRUE)
        return;
    PumpSnapshot oldest;
    xQueueReceive(stateQueue_, &oldest, 0);
    xQueueSendToBack(stateQueue_, &state, 0);
}

void PumpController::taskEntry(void *context) {
    static_cast<PumpController *>(context)->run();
}

void PumpController::run() {
    uint32_t wateringCount = 0;
    for (;;) {
        uint32_t durationMs = 0;
        if (xQueueReceive(requestQueue_, &durationMs, portMAX_DELAY) != pdTRUE)
            continue;
        if (durationMs < AppConfig::kMinPumpTimeMs || durationMs > AppConfig::kMaxWaterTimeMs) {
            digitalWrite(AppConfig::kPumpPin, LOW);
            Serial.println("[Pump] Khong can tuoi.");
            continue;
        }

        Serial.printf("[Pump] Bat %lu ms\n", (unsigned long)durationMs);
        digitalWrite(AppConfig::kPumpPin, HIGH);
        enqueueState(PumpSnapshot{true, wateringCount, millis()});
        vTaskDelay(pdMS_TO_TICKS(durationMs));
        digitalWrite(AppConfig::kPumpPin, LOW);
        ++wateringCount;
        enqueueState(PumpSnapshot{false, wateringCount, millis()});
        xQueueOverwrite(resultQueue_, &durationMs);
        Serial.println("[Pump] Tat.");
    }
}
