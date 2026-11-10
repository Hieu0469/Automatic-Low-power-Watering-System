#pragma once

#include <Arduino.h>

/** @file AppConfig.h @brief Các hằng số phần cứng và thuật toán của firmware. */
namespace AppConfig {
constexpr uint8_t kDhtPin = 15;
constexpr uint8_t kPumpPin = 4;
constexpr uint32_t kSensorIntervalMs = 10000;
constexpr uint32_t kMaxWaterTimeMs = 1500;
constexpr uint32_t kMinPumpTimeMs = 500;
constexpr uint32_t kPumpStopThresholdMs = 350;
constexpr uint8_t kPumpConfirmCycles = 2;
constexpr float kSoilEmaAlpha = 0.35f;

constexpr float kSoilDryThresholdPct = 40.0f;
constexpr float kTankMinRisePct = 1.0f;
constexpr int kTankFailCount = 2;
constexpr uint32_t kAlertRepeatMs = 30000;
constexpr uint32_t kAppLoopDelayMs = 200;
} // namespace AppConfig
