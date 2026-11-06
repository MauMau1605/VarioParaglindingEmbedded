/// @file main.cpp
/// @brief Zephyr application entry point for VarioGPS-BLE.

#include <zephyr/kernel.h>
#include "app/vario_app.hpp"
#include "hal/pwm_hal.hpp"

int main(void)
{
    const vario::Error err = vario::app::vario_app_init();
    if (err != vario::Error::kOk) {
        // FATAL only (PWM/BMP390/engine). Watchdog is not armed yet: no feeding needed.
        while (true) {
            for (uint8_t i = 0; i < 3; ++i) {
                (void)vario::hal::pwm_set_frequency(2000);
                k_msleep(100);
                (void)vario::hal::pwm_stop();
                k_msleep(100);
            }
            k_msleep(1400);
        }
    }

    vario::app::vario_app_run();
    return 0;
}
