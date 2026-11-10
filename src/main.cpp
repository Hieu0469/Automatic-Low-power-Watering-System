#include <Arduino.h>

#include "WateringApp.h"

/** @brief Arduino entry points; toàn bộ logic nằm trong các module riêng. */
static WateringApp app;

void setup() {
    app.begin();
}

void loop() {
    app.tick();
}
