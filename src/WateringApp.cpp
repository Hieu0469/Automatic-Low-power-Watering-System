#include "WateringApp.h"

#include "AppConfig.h"
#include "BLE_server.h"

WateringApp::WateringApp() : sensors_(config_, pump_) {}

void WateringApp::begin() {
    Serial.begin(9600);
    delay(500);
    Serial.println("=== He Thong Tuoi Tu Dong ===");

    // Chân bơm luôn về LOW kể cả khi khởi tạo module thất bại.
    pinMode(AppConfig::kPumpPin, OUTPUT);
    digitalWrite(AppConfig::kPumpPin, LOW);
    if (!config_.begin() || !pump_.begin()) {
        Serial.println("[App] Khong khoi tao duoc config/pump; dung he thong.");
        return;
    }

    ble_server_init(config_);
    if (!sensors_.begin()) {
        Serial.println("[App] Khong khoi tao duoc sensor; dung he thong.");
        return;
    }
    ready_ = true;
}

void WateringApp::updateAlert(const TelemetrySnapshot &sample) {
    if (!sample.sensorValid)
        return;
    if (!alertKnown_ || sample.noWaterAlert != tankAlertActive_) {
        const bool wasActive = tankAlertActive_;
        tankAlertActive_ = sample.noWaterAlert;
        alertKnown_ = true;
        if (tankAlertActive_ || wasActive) {
            ble_send_alert_msg(tankAlertActive_ ? "ALERT:NO_WATER" : "ALERT:CLEAR");
        }
        lastAlertRepeatMs_ = millis();
    }
}

void WateringApp::serviceBle() {
    if (ble_config_requested()) {
        String json;
        if (config_.toJSON(json)) {
            ble_clear_config_request();
            ble_send_config(json);
        }
    }
    if (tankAlertActive_ && millis() - lastAlertRepeatMs_ >= AppConfig::kAlertRepeatMs) {
        lastAlertRepeatMs_ = millis();
        ble_send_alert_msg("ALERT:NO_WATER");
    }
}

void WateringApp::tick() {
    if (!ready_) {
        delay(1000);
        return;
    }

    TelemetrySnapshot sample;
    if (sensors_.pollTelemetry(sample)) {
        if (sample.sensorValid) {
            ble_send_sensor_data(sample.temperatureC, sample.airHumidityPct, sample.soilMoisturePct,
                                 (int)sample.pumpCommandMs, sample.noWaterFailCount);
        }
        updateAlert(sample);
    }

    PumpSnapshot pumpState;
    while (pump_.pollState(pumpState)) {
        // Đọc hết queue; PumpController đã ghi log trạng thái bơm.
    }

    serviceBle();
    vTaskDelay(pdMS_TO_TICKS(AppConfig::kAppLoopDelayMs));
}
