/// @file vario_app.cpp
/// @brief Application orchestrator implementation.
///        Manages timing and data flow: BMP390 → Vario Engine → LK8EX1 → USB.

#include "app/vario_app.hpp"
#include "hal/i2c_hal.hpp"
#include "hal/usb_serial_hal.hpp"
#include "drivers/bmp390.hpp"
#include "middleware/vario_engine.hpp"
#include "middleware/lk8ex1_formatter.hpp"
#include "common/config.hpp"
#include "common/types.hpp"

#include <Arduino.h>  // millis()

namespace vario {
namespace app {

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------
static bool     s_initialized = false;
static uint32_t s_last_acquisition_ms = 0;
static uint32_t s_last_transmit_ms = 0;
static VarioData s_latest_vario = {};

Error vario_app_init()
{
    // 1. Initialize I2C bus
    Error err = hal::i2c_init();
    if (err != Error::kOk) {
        return err;
    }

    // 2. Initialize USB Serial (non-blocking, works without host)
    err = hal::usb_serial_init(config::kUsbBaudRate);
    if (err != Error::kOk) {
        return err;
    }

    // 3. Initialize BMP390 sensor
    err = bmp390::init();
    if (err != Error::kOk) {
        return err;
    }

    // 4. Initialize vario engine
    err = middleware::vario_engine_init();
    if (err != Error::kOk) {
        return err;
    }

    s_last_acquisition_ms = millis();
    s_last_transmit_ms = millis();
    s_initialized = true;

    return Error::kOk;
}

Error vario_app_run()
{
    if (!s_initialized) {
        return Error::kNotInitialized;
    }

    const uint32_t now_ms = millis();

    // --- Pressure acquisition at 50 Hz (every 20 ms) ---
    if ((now_ms - s_last_acquisition_ms) >= config::kAcquisitionIntervalMs) {
        s_last_acquisition_ms = now_ms;

        PressureData pressure = {};
        Error err = bmp390::read_pressure_temperature(&pressure);
        if (err != Error::kOk) {
            return err;
        }

        err = middleware::vario_engine_update(&pressure, &s_latest_vario);
        if (err != Error::kOk) {
            return err;
        }
    }

    // --- LK8EX1 transmission at 10 Hz (every 100 ms) ---
    if ((now_ms - s_last_transmit_ms) >= config::kTransmitIntervalMs) {
        s_last_transmit_ms = now_ms;

        // Only transmit if USB host is connected
        if (hal::usb_serial_is_connected()) {
            char sentence[config::kLk8ex1MaxLen] = {};
            uint8_t sentence_len = 0;

            const Error err = middleware::lk8ex1_format(
                &s_latest_vario, sentence, config::kLk8ex1MaxLen, &sentence_len);
            if (err != Error::kOk) {
                return err;
            }

            const Error tx_err = hal::usb_serial_write(sentence, sentence_len);
            if (tx_err != Error::kOk) {
                return tx_err;
            }
        }
    }

    return Error::kOk;
}

}  // namespace app
}  // namespace vario
