#pragma once

#include <Arduino.h>

#include "FuzzyConfig.h"

/** @file FuzzyLogic.h @brief Hàm suy luận mờ thuần, không giữ trạng thái toàn cục. */

namespace FuzzyLogic {
constexpr int kWaterNone = 0;
constexpr int kWaterLow = 1;
constexpr int kWaterHigh = 2;

/**
 * @brief Tính mức thuộc của x trong hàm tam giác/vai [a,b,c].
 * @return Giá trị trong [0,1] khi a ≤ b ≤ c.
 */
inline float triangle(float x, float a, float b, float c) {
    if (x <= a)
        return (a == b) ? 1.0f : 0.0f;
    if (x >= c)
        return (b == c) ? 1.0f : 0.0f;
    if (x <= b)
        return (b == a) ? 1.0f : (x - a) / (b - a);
    return (c == b) ? 1.0f : (c - x) / (c - b);
}
} // namespace FuzzyLogic

/**
 * @brief Suy luận 27 luật theo thứ tự [soil][humidity][temperature].
 * @return Mức tưới 0–100%.
 *
 * Hàm không sửa cấu hình; ConfigService giữ mutex khi truyền params vào.
 */
inline float fuzzy_defuzzify(const FuzzyParams &p, float temp, float hum, float soil) {
    const float t[3] = {FuzzyLogic::triangle(temp, p.temp_hot.a, p.temp_hot.b, p.temp_hot.c),
                        FuzzyLogic::triangle(temp, p.temp_med.a, p.temp_med.b, p.temp_med.c),
                        FuzzyLogic::triangle(temp, p.temp_cold.a, p.temp_cold.b, p.temp_cold.c)};
    const float h[3] = {FuzzyLogic::triangle(hum, p.hum_low.a, p.hum_low.b, p.hum_low.c),
                        FuzzyLogic::triangle(hum, p.hum_med.a, p.hum_med.b, p.hum_med.c),
                        FuzzyLogic::triangle(hum, p.hum_hi.a, p.hum_hi.b, p.hum_hi.c)};
    const float s[3] = {FuzzyLogic::triangle(soil, p.soil_low.a, p.soil_low.b, p.soil_low.c),
                        FuzzyLogic::triangle(soil, p.soil_med.a, p.soil_med.b, p.soil_med.c),
                        FuzzyLogic::triangle(soil, p.soil_hi.a, p.soil_hi.b, p.soil_hi.c)};

    float high = 0.0f;
    float low = 0.0f;
    float none = 0.0f;
    for (int soilIndex = 0; soilIndex < 3; ++soilIndex) {
        for (int humIndex = 0; humIndex < 3; ++humIndex) {
            for (int tempIndex = 0; tempIndex < 3; ++tempIndex) {
                const float firing = min(min(t[tempIndex], h[humIndex]), s[soilIndex]);
                const int output = p.rules[soilIndex][humIndex][tempIndex];
                if (output == FuzzyLogic::kWaterHigh)
                    high = max(high, firing);
                else if (output == FuzzyLogic::kWaterLow)
                    low = max(low, firing);
                else
                    none = max(none, firing);
            }
        }
    }

    const float total = high + low + none;
    return total == 0.0f ? 0.0f : (high * 100.0f + low * 50.0f) / total;
}
