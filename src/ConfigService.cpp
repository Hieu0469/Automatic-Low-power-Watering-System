#include "ConfigService.h"

#include "FuzzyLogic.h"

bool ConfigService::begin() {
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_)
        return false;
    config_.begin();
    return true;
}

bool ConfigService::calculateWaterPercent(float temp, float hum, float soil, float &result) {
    if (!mutex_ || xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE)
        return false;
    result = fuzzy_defuzzify(config_.params, temp, hum, soil);
    xSemaphoreGive(mutex_);
    return true;
}

ConfigUpdateResult ConfigService::applyCommand(const String &command) {
    if (!mutex_ || xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ConfigUpdateResult::Busy;
    const bool applied = config_.applyCommand(command);
    xSemaphoreGive(mutex_);
    return applied ? ConfigUpdateResult::Applied : ConfigUpdateResult::Invalid;
}

ConfigUpdateResult ConfigService::applyJson(JsonVariantConst json) {
    if (!mutex_ || xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE)
        return ConfigUpdateResult::Busy;
    const bool applied = config_.applyJson(json);
    xSemaphoreGive(mutex_);
    return applied ? ConfigUpdateResult::Applied : ConfigUpdateResult::Invalid;
}

bool ConfigService::toJSON(String &json) {
    if (!mutex_ || xSemaphoreTake(mutex_, pdMS_TO_TICKS(1000)) != pdTRUE)
        return false;
    json = config_.toJSON();
    xSemaphoreGive(mutex_);
    return true;
}
