#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "Telemetry.h"

/** @brief Sở hữu chân bơm và task thực thi xung tưới. */
class PumpController {
  public:
    /** @brief Tạo queue/task và đặt chân bơm về LOW trước khi chạy. */
    bool begin();

    /** @brief Gửi lệnh mới nhất; 0 ms nghĩa là không tưới. */
    void request(uint32_t durationMs);

    /** @return Thời gian bơm đã chạy kể từ lần lấy kết quả trước, hoặc 0. */
    uint32_t takeLastRunMs();

    /** @brief Lấy sự kiện bơm bật/tắt theo thứ tự thời gian. */
    bool pollState(PumpSnapshot &state);

  private:
    static void taskEntry(void *context);
    void run();
    void enqueueState(const PumpSnapshot &state);

    QueueHandle_t requestQueue_ = nullptr;
    QueueHandle_t resultQueue_ = nullptr;
    QueueHandle_t stateQueue_ = nullptr;
    TaskHandle_t taskHandle_ = nullptr;
};
