#include "WateringApp.h"

#include "AppConfig.h"
#include "BLE_server.h"

WateringApp::WateringApp() : sensors_(config_, pump_), mqtt_(config_) {}

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
    mqtt_.begin();
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

    mqtt_.tick();
    const bool reconnected = mqtt_.takeJustConnected();
    if (reconnected) {
        telemetryPending_ = hasTelemetry_;
        pumpPending_ = hasPump_;
    }

    TelemetrySnapshot sample;
    if (sensors_.pollTelemetry(sample)) {
        latestTelemetry_ = sample;
        hasTelemetry_ = true;
        telemetryPending_ = true;
        if (sample.sensorValid) {
            ble_send_sensor_data(sample.temperatureC, sample.airHumidityPct, sample.soilMoisturePct,
                                 (int)sample.pumpCommandMs, sample.noWaterFailCount);
        }
        updateAlert(sample);
    }

    PumpSnapshot pumpState;
    while (pump_.pollState(pumpState)) {
        latestPump_ = pumpState;
        hasPump_ = true;
        pumpPending_ = true;
        if (mqtt_.connected() && !reconnected && mqtt_.publishPumpState(pumpState))
            pumpPending_ = false;
    }

    if (mqtt_.connected()) {
        if (hasTelemetry_ && telemetryPending_ && mqtt_.publishTelemetry(latestTelemetry_))
            telemetryPending_ = false;
        if (hasPump_ && pumpPending_ && mqtt_.publishPumpState(latestPump_))
            pumpPending_ = false;
        if (ble_config_changed() && mqtt_.publishConfig())
            ble_clear_config_changed();
    }

    serviceBle();
    vTaskDelay(pdMS_TO_TICKS(AppConfig::kAppLoopDelayMs));
}
