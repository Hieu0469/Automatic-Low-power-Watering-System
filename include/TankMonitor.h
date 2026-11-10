#pragma once

#include <Arduino.h>

/** @brief Suy đoán khả năng hết nước từ độ tăng ẩm sau lần bơm đã chạy. */
class TankMonitor {
  public:
    /** @brief Cập nhật trạng thái cảnh báo sau một phép đo hợp lệ. */
    void update(float filteredSoilPct, float rawSoilPct, uint32_t lastPumpMs);

    /** @brief Bỏ mốc so sánh cũ sau lỗi cảm biến; giữ nguyên cảnh báo hiện có. */
    void invalidate();

    /** @return Số lần tưới liên tiếp không làm độ ẩm tăng đủ ngưỡng. */
    int failCount() const {
        return failCount_;
    }

    /** @return true khi đang cảnh báo có thể hết nước. */
    bool alertActive() const {
        return alertActive_;
    }

  private:
    float previousRawSoilPct_ = -1.0f;
    int failCount_ = 0;
    bool alertActive_ = false;
};
