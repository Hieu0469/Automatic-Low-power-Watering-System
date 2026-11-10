#include "TankMonitor.h"

#include "AppConfig.h"

void TankMonitor::update(float filteredSoilPct, float rawSoilPct, uint32_t lastPumpMs) {
    if (filteredSoilPct <= AppConfig::kSoilDryThresholdPct && previousRawSoilPct_ >= 0.0f &&
        lastPumpMs >= AppConfig::kMinPumpTimeMs) {
        const float rise = rawSoilPct - previousRawSoilPct_;
        if (rise < AppConfig::kTankMinRisePct) {
            ++failCount_;
            Serial.printf("[Tank] Soil %.1f%% -> %.1f%%, fail=%d/%d\n", previousRawSoilPct_,
                          rawSoilPct, failCount_, AppConfig::kTankFailCount);
            if (failCount_ >= AppConfig::kTankFailCount && !alertActive_) {
                alertActive_ = true;
                Serial.println("[Tank] CANH BAO: Co the het nuoc!");
            }
        } else {
            failCount_ = 0;
            if (alertActive_)
                Serial.println("[Tank] Do am tang, xoa canh bao.");
            alertActive_ = false;
        }
    } else if (filteredSoilPct > AppConfig::kSoilDryThresholdPct) {
        failCount_ = 0;
        alertActive_ = false;
    }
    previousRawSoilPct_ = rawSoilPct;
}

void TankMonitor::invalidate() {
    previousRawSoilPct_ = -1.0f;
}
