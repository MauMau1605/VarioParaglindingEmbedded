/// @file main.cpp
/// @brief VarioUSB entry point — Arduino setup() and loop().
///        Delegates all logic to vario_app.

#include "app/vario_app.hpp"
#include <Arduino.h>

/// @brief Arduino setup — initialize all subsystems.
///        On failure, blinks the built-in LED rapidly as error indicator.
void setup()
{
    // Configure LED for error indication
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);  // LED off (active low on XIAO)

    const vario::Error err = vario::app::vario_app_init();
    if (err != vario::Error::kOk) {
        // Init failed: enter error blink loop (never returns)
        while (true) {
            digitalWrite(LED_BUILTIN, LOW);   // LED on
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);  // LED off
            delay(100);
        }
    }

    // Success: brief LED flash to confirm
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
}

/// @brief Arduino main loop — runs the vario application tick.
void loop()
{
    vario::app::vario_app_run();
}
