/// @file main.cpp
/// @brief VarioUSB entry point — Arduino setup() and loop().
///        Delegates all logic to vario_app.

#include "app/vario_app.hpp"
#include "drivers/bmp390.hpp"
#include "common/config.hpp"
#include <Arduino.h>

/// @brief Helper returning a human-readable string for an error code.
static const char* error_to_string(const vario::Error err)
{
    switch (err) {
        case vario::Error::kOk:             return "OK";
        case vario::Error::kTimeout:        return "TIMEOUT";
        case vario::Error::kNack:           return "NACK (Device not responding)";
        case vario::Error::kBusy:           return "BUSY";
        case vario::Error::kInvalidData:    return "INVALID_DATA";
        case vario::Error::kNotInitialized: return "NOT_INITIALIZED";
        case vario::Error::kInvalidChipId:  return "INVALID_CHIP_ID (Not BMP390/BMP388)";
        case vario::Error::kWriteFailed:    return "WRITE_FAILED";
        case vario::Error::kReadFailed:     return "READ_FAILED";
        default:                            return "UNKNOWN_ERROR";
    }
}

/// @brief Print startup banner and hardware bus configuration.
static void print_boot_banner()
{
    Serial.println(F("\n=========================================="));
    Serial.println(F("           VarioUSB Variometer            "));
    Serial.println(F("=========================================="));

#if VARIO_USE_SPI
    Serial.println(F("[BUS] Mode: SPI (4-wire)"));
    Serial.println(F("[BUS] Pins: SCK=Pin 8, MISO=Pin 9, MOSI=Pin 10, CS=Pin 3"));
#else
    Serial.println(F("[BUS] Mode: I2C"));
    Serial.print(F("[BUS] Pins: SDA=Pin 4, SCL=Pin 5, Target Address: 0x"));
    Serial.println(vario::config::kBmp390I2cAddress, HEX);
#endif
}

/// @brief Enter infinite error blink loop on boot failure.
static void enter_error_loop(const vario::Error err)
{
    const uint8_t chip_id = vario::bmp390::get_detected_chip_id();

    Serial.print(F("[BOOT ERROR] Init failed: "));
    Serial.print(error_to_string(err));
    Serial.print(F(" (code "));
    Serial.print(static_cast<int>(err));
    Serial.print(F("), Detected Chip ID: 0x"));
    if (chip_id < 16) Serial.print('0');
    Serial.println(chip_id, HEX);

    while (true) {
        digitalWrite(LED_BUILTIN, LOW);   // LED on (active low on XIAO)
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);  // LED off
        delay(900);

        Serial.print(F("$ERR,INIT_FAILED,"));
        Serial.print(error_to_string(err));
        Serial.print(F(",CHIP=0x"));
        if (chip_id < 16) Serial.print('0');
        Serial.println(chip_id, HEX);
    }
}

/// @brief Arduino setup — initializes subsystems and reports status.
void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);  // LED off (active low)

    // Allow host USB time to connect (up to 2 seconds)
    Serial.begin(vario::config::kUsbBaudRate);
    const uint32_t start_ms = millis();
    while (!Serial && (millis() - start_ms) < 2000) {}

    // Allow sensor power rail and power-on reset (POR) to fully stabilize
    delay(100);

    print_boot_banner();

    const vario::Error err = vario::app::vario_app_init();
    if (err != vario::Error::kOk) {
        enter_error_loop(err);
    }

    const uint8_t chip_id = vario::bmp390::get_detected_chip_id();
    Serial.print(F("[BOOT OK] Sensor online! Chip ID: 0x"));
    if (chip_id < 16) Serial.print('0');
    Serial.print(chip_id, HEX);
    Serial.println(F(" -> Streaming LK8EX1 sentences..."));
    Serial.println(F("==========================================\n"));

    // Brief LED flash to confirm successful boot
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
}

/// @brief Arduino main loop — runs periodic sensor acquisition and NMEA streaming.
void loop()
{
    vario::app::vario_app_run();
}
