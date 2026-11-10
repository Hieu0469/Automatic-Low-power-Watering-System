#pragma once
#include <Arduino.h>

/** @file SoilSensor.h @brief Đọc ADC độ ẩm đất và loại mẫu nhiễu đột biến. */

// ─── Cấu hình chân — giữ nguyên như ESP-IDF gốc ─────────────────────────────
// D0 → GPIO5  (digital)
// A0 → GPIO34 (analog, ADC1_CH6)

#define SOIL_D0_PIN 5
#define SOIL_A0_PIN 34

// Ngưỡng ADC (0–4095): hiệu chỉnh theo thực tế
// raw thấp = ẩm, raw cao = khô
#define SOIL_THRESHOLD_WET 1200
#define SOIL_THRESHOLD_DRY 2800

// Lấy 16 mẫu và dùng trung vị để loại các xung nhiễu ADC.
#define SOIL_ADC_SAMPLES 16
#define SOIL_ADC_RAIL_LOW 16
#define SOIL_ADC_RAIL_HIGH 4079

/** @brief Một phép đo đất gồm ADC thô, phần trăm quy đổi và cờ hợp lệ. */
struct SoilReading {
    int raw;
    float moisturePct;
    bool valid;
};

/** @brief Driver cảm biến độ ẩm đất trên GPIO34 (analog) và GPIO5 (digital). */
class SoilSensor {
  public:
    /** @brief Thiết lập chân digital của module cảm biến. */
    void begin() {
        pinMode(SOIL_D0_PIN, INPUT_PULLUP);
        // GPIO34 chỉ dùng analogRead, không cần pinMode
    }

    /**
     * @brief Đọc độ ẩm đất bằng trung vị 16 mẫu, sau đó quy đổi sang 0–100%.
     * @return Cờ valid=false khi ADC sát biên; không thể phát hiện mọi dây hở.
     */
    SoilReading read() {
        const int raw = _readRawMedian();
        if (raw <= SOIL_ADC_RAIL_LOW || raw >= SOIL_ADC_RAIL_HIGH)
            return {raw, NAN, false};

        // Map ngược: raw thấp = ẩm = 100%, raw cao = khô = 0%.
        const float pct = 100.0f - (float)(raw - SOIL_THRESHOLD_WET) /
                                       (SOIL_THRESHOLD_DRY - SOIL_THRESHOLD_WET) * 100.0f;
        return {raw, constrain(pct, 0.0f, 100.0f), true};
    }

    /** @return true khi ngõ D0 ở LOW (ngưỡng module LM393). */
    bool readDigital() {
        return (digitalRead(SOIL_D0_PIN) == LOW);
    }

    /** @return Trung vị ADC thô, dùng khi hiệu chuẩn cảm biến sau này. */
    int readRaw() {
        return _readRawMedian();
    }

  private:
    /** @brief Sắp 16 mẫu ADC tại chỗ và lấy trung vị. */
    int _readRawMedian() {
        int samples[SOIL_ADC_SAMPLES];
        for (int i = 0; i < SOIL_ADC_SAMPLES; i++) {
            samples[i] = analogRead(SOIL_A0_PIN);
            delayMicroseconds(500);
        }
        // Insertion sort đủ nhanh với 16 mẫu và không cần cấp phát động.
        for (int i = 1; i < SOIL_ADC_SAMPLES; i++) {
            const int value = samples[i];
            int j = i;
            while (j > 0 && samples[j - 1] > value) {
                samples[j] = samples[j - 1];
                --j;
            }
            samples[j] = value;
        }
        return (samples[SOIL_ADC_SAMPLES / 2 - 1] + samples[SOIL_ADC_SAMPLES / 2]) / 2;
    }
};
