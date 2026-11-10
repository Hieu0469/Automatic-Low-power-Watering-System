#include "MqttService.h"

#include <ArduinoJson.h>

#include "ConfigService.h"

#if __has_include("MqttSettings.h")
#include "MqttSettings.h"
#else
#include "MqttSettings.example.h"
#endif

namespace {
constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kMqttRetryMs = 5000;
constexpr uint16_t kMqttPacketBytes = 2048;

/** @brief Giới hạn ký tự trong device ID và request ID dùng làm topic/ACK. */
bool validToken(const char *id, size_t maxLength) {
    if (!id || !id[0])
        return false;
    size_t length = 0;
    for (; id[length] != '\0'; ++length) {
        const char c = id[length];
        if (length >= maxLength || !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                     (c >= '0' && c <= '9') || c == '-' || c == '_')) {
            return false;
        }
    }
    return length > 0;
}
} // namespace

MqttService *MqttService::active_ = nullptr;

MqttService::MqttService(ConfigService &config) : config_(config), mqtt_(wifiClient_) {}

void MqttService::begin() {
    enabled_ = MqttSettings::kWifiSsid[0] && MqttSettings::kBrokerHost[0] &&
               validToken(MqttSettings::kDeviceId, 40);
    if (!enabled_) {
        Serial.println("[MQTT] Chua cau hinh MqttSettings.h; MQTT tam tat.");
        return;
    }

    baseTopic_ = String("watering/") + MqttSettings::kDeviceId;
    statusTopic_ = baseTopic_ + "/status";
    telemetryTopic_ = baseTopic_ + "/telemetry";
    pumpTopic_ = baseTopic_ + "/pump/state";
    commandTopic_ = baseTopic_ + "/config/set";
    configStateTopic_ = baseTopic_ + "/config/state";
    ackTopic_ = baseTopic_ + "/config/ack";
    active_ = this;
    mqtt_.setServer(MqttSettings::kBrokerHost, MqttSettings::kBrokerPort);
    mqtt_.setCallback(onMessageStatic);
    mqtt_.setKeepAlive(30);
    mqtt_.setSocketTimeout(2);
    if (!mqtt_.setBufferSize(kMqttPacketBytes)) {
        enabled_ = false;
        Serial.println("[MQTT] Khong cap phat duoc MQTT buffer.");
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(MqttSettings::kWifiSsid, MqttSettings::kWifiPassword);
    lastWifiAttemptMs_ = millis();
    Serial.println("[MQTT] Dang ket noi Wi-Fi...");
}

void MqttService::tick() {
    if (!enabled_)
        return;
    const uint32_t now = millis();
    if (WiFi.status() != WL_CONNECTED) {
        justConnected_ = false;
        if (now - lastWifiAttemptMs_ >= kWifiRetryMs) {
            lastWifiAttemptMs_ = now;
            WiFi.disconnect();
            WiFi.begin(MqttSettings::kWifiSsid, MqttSettings::kWifiPassword);
            Serial.println("[MQTT] Thu ket noi lai Wi-Fi.");
        }
        return;
    }

    if (!mqtt_.connected()) {
        if (lastMqttAttemptMs_ != 0 && now - lastMqttAttemptMs_ < kMqttRetryMs)
            return;
        lastMqttAttemptMs_ = now;
        const String clientId = String(MqttSettings::kDeviceId) + "-fw";
        const char *user = MqttSettings::kBrokerUser[0] ? MqttSettings::kBrokerUser : nullptr;
        const char *pass = user ? MqttSettings::kBrokerPassword : nullptr;
        if (!mqtt_.connect(clientId.c_str(), user, pass, statusTopic_.c_str(), 1, true,
                           "offline")) {
            Serial.printf("[MQTT] Broker chua san sang (state=%d).\n", mqtt_.state());
            return;
        }
        if (!mqtt_.subscribe(commandTopic_.c_str(), 1)) {
            Serial.println("[MQTT] Khong subscribe duoc config/set.");
            mqtt_.disconnect();
            return;
        }
        publish(statusTopic_, "online", true);
        publishConfig();
        justConnected_ = true;
        Serial.printf("[MQTT] Da ket noi: %s\n", baseTopic_.c_str());
    }
    mqtt_.loop();
}

bool MqttService::connected() {
    return enabled_ && WiFi.status() == WL_CONNECTED && mqtt_.connected();
}

bool MqttService::takeJustConnected() {
    const bool result = justConnected_;
    justConnected_ = false;
    return result;
}

bool MqttService::publish(const String &name, const char *payload, bool retained) {
    return connected() && mqtt_.publish(name.c_str(), payload, retained);
}

bool MqttService::publishTelemetry(const TelemetrySnapshot &sample) {
    char payload[320];
    int length = 0;
    if (sample.sensorValid) {
        length =
            snprintf(payload, sizeof(payload),
                     "{\"v\":1,\"uptimeMs\":%lu,\"valid\":true,\"temperatureC\":%.1f,"
                     "\"airHumidityPct\":%.1f,\"soilMoisturePct\":%.1f,\"soilRaw\":%d,"
                     "\"pumpCommandMs\":%lu,\"noWaterFailCount\":%d,\"noWaterAlert\":%s}",
                     (unsigned long)sample.uptimeMs, sample.temperatureC, sample.airHumidityPct,
                     sample.soilMoisturePct, sample.soilRaw, (unsigned long)sample.pumpCommandMs,
                     sample.noWaterFailCount, sample.noWaterAlert ? "true" : "false");
    } else {
        length = snprintf(payload, sizeof(payload),
                          "{\"v\":1,\"uptimeMs\":%lu,\"valid\":false,\"error\":\"%s\"}",
                          (unsigned long)sample.uptimeMs, sample.error);
    }
    if (length < 0 || length >= (int)sizeof(payload))
        return false;
    return publish(telemetryTopic_, payload, true);
}

bool MqttService::publishPumpState(const PumpSnapshot &state) {
    char payload[128];
    const int length = snprintf(payload, sizeof(payload),
                                "{\"v\":1,\"uptimeMs\":%lu,\"pumpOn\":%s,\"wateringCount\":%lu}",
                                (unsigned long)state.uptimeMs, state.pumpOn ? "true" : "false",
                                (unsigned long)state.wateringCount);
    if (length < 0 || length >= (int)sizeof(payload))
        return false;
    return publish(pumpTopic_, payload, true);
}

bool MqttService::publishConfig() {
    if (!connected())
        return false;
    String json;
    if (!config_.toJSON(json))
        return false;
    return publish(configStateTopic_, json.c_str(), true);
}

void MqttService::onMessageStatic(char *name, uint8_t *payload, unsigned int length) {
    if (active_)
        active_->onMessage(name, payload, length);
}

void MqttService::onMessage(const char *name, const uint8_t *payload, unsigned int length) {
    if (commandTopic_ != name)
        return;
    if (length > 1500) {
        publishAck("", false, "payload_too_large");
        return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, payload, length)) {
        publishAck("", false, "invalid_json");
        return;
    }
    const char *requestId = doc["requestId"] | "";
    const char *action = doc["action"] | "";
    if (!validToken(requestId, 48)) {
        publishAck("", false, "invalid_request_id");
        return;
    }

    bool ok = false;
    const char *reason = "unknown_action";
    if (strcmp(action, "get") == 0) {
        ok = publishConfig();
        reason = ok ? "ok" : "publish_failed";
    } else if (strcmp(action, "set") == 0 || strcmp(action, "reset") == 0) {
        const ConfigUpdateResult result =
            strcmp(action, "reset") == 0 ? config_.applyCommand("RESET")
                                         : config_.applyJson(doc["config"].as<JsonVariantConst>());
        ok = result == ConfigUpdateResult::Applied;
        reason = result == ConfigUpdateResult::Busy ? "config_busy" : ok ? "ok" : "invalid_config";
        if (ok)
            publishConfig();
    }
    publishAck(requestId, ok, reason);
}

void MqttService::publishAck(const char *requestId, bool ok, const char *reason) {
    JsonDocument doc;
    doc["v"] = 1;
    doc["requestId"] = requestId;
    doc["ok"] = ok;
    doc["reason"] = reason;
    char payload[192];
    if (serializeJson(doc, payload, sizeof(payload)) == 0)
        return;
    publish(ackTopic_, payload, false);
}
