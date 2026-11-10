#pragma once
/*
 * Wrapper mỏng cho Adafruit DHT library
 * Dùng thư viện Adafruit vì nó xử lý timing DHT22 bằng
 * cách tắt interrupt trong lúc đọc — ổn định nhất trên ESP32
 *
 * Cài qua platformio.ini:
 *   adafruit/DHT sensor library @ ^1.4.6
 *   adafruit/Adafruit Unified Sensor @ ^1.1.14
 */
#include <Arduino.h>
#include <DHT.h>

/** @brief Driver DHT22 với kiểm tra miền giá trị hợp lệ. */
class DHT22Driver {
  public:
    /** @param pin GPIO data của DHT22. */
    DHT22Driver(uint8_t pin) : _dht(pin, DHT22) {}

    /** @brief Khởi động thư viện Adafruit DHT. */
    void begin() {
        _dht.begin();
        Serial.println("[DHT22] Khoi tao xong.");
    }

    /**
     * @brief Đọc nhiệt độ và độ ẩm; thư viện cache kết quả trong 2 giây.
     * @return false nếu NaN, vô cực hoặc ngoài dải DHT22.
     */
    bool read() {
        _temp = _dht.readTemperature();
        _hum = _dht.readHumidity();

        if (!isfinite(_temp) || !isfinite(_hum) || _temp < -40.0f || _temp > 80.0f || _hum < 0.0f ||
            _hum > 100.0f) {
            Serial.println("[DHT22] Du lieu khong hop le — kiem tra day noi GPIO15");
            return false;
        }
        return true;
    }

    /** @return Nhiệt độ °C của lần đọc gần nhất; chỉ dùng sau khi read() thành công. */
    float temperature() {
        return _temp;
    }
    /** @return Độ ẩm không khí % của lần đọc gần nhất; chỉ dùng sau khi read() thành công. */
    float humidity() {
        return _hum;
    }

  private:
    DHT _dht;
    float _temp = NAN;
    float _hum = NAN;
};
