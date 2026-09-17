/// @file bmp390.cpp
/// @brief BMP390 driver implementation — init, calibration, compensated readout.
///        Compensation algorithm based on Bosch BMP3-Sensor-API (integer math).

#include "drivers/bmp390.hpp"
#include "drivers/bmp390_defs.hpp"
#include "hal/i2c_hal.hpp"
#include "common/config.hpp"

#include <Arduino.h>  // delay()

namespace vario {
namespace bmp390 {

// ---------------------------------------------------------------------------
// Module-level static state (no heap, no globals outside this TU)
// ---------------------------------------------------------------------------
static CalibData s_calib = {};
static int64_t   s_t_lin = 0;  // Linearized temperature for pressure compensation
static bool      s_initialized = false;

// ---------------------------------------------------------------------------
// Internal helper: parse calibration bytes into CalibData
// ---------------------------------------------------------------------------
static void parse_calib_data(const uint8_t* raw)
{
    s_calib.par_t1  = static_cast<uint16_t>(raw[1] << 8 | raw[0]);
    s_calib.par_t2  = static_cast<uint16_t>(raw[3] << 8 | raw[2]);
    s_calib.par_t3  = static_cast<int8_t>(raw[4]);

    s_calib.par_p1  = static_cast<int16_t>(raw[6] << 8 | raw[5]);
    s_calib.par_p2  = static_cast<int16_t>(raw[8] << 8 | raw[7]);
    s_calib.par_p3  = static_cast<int8_t>(raw[9]);
    s_calib.par_p4  = static_cast<int8_t>(raw[10]);
    s_calib.par_p5  = static_cast<uint16_t>(raw[12] << 8 | raw[11]);
    s_calib.par_p6  = static_cast<uint16_t>(raw[14] << 8 | raw[13]);
    s_calib.par_p7  = static_cast<int8_t>(raw[15]);
    s_calib.par_p8  = static_cast<int8_t>(raw[16]);
    s_calib.par_p9  = static_cast<int16_t>(raw[18] << 8 | raw[17]);
    s_calib.par_p10 = static_cast<int8_t>(raw[19]);
    s_calib.par_p11 = static_cast<int8_t>(raw[20]);
}

// ---------------------------------------------------------------------------
// Internal helper: compensate temperature (Bosch algorithm, 64-bit integer)
// Returns temperature in centidegrees Celsius (e.g., 2150 = 21.50 °C)
// ---------------------------------------------------------------------------
static int32_t compensate_temperature(const uint32_t raw_temp)
{
    const int64_t partial_data1 = static_cast<int64_t>(raw_temp) -
                                  (static_cast<int64_t>(s_calib.par_t1) << 8);

    const int64_t partial_data2 = partial_data1 *
                                  static_cast<int64_t>(s_calib.par_t2);

    // Store linearized temperature for pressure compensation
    s_t_lin = partial_data2 +
              ((partial_data1 * partial_data1) *
               static_cast<int64_t>(s_calib.par_t3)) / (1LL << 48);

    // Return compensated temperature in centidegrees
    return static_cast<int32_t>(s_t_lin / (1LL << 30));  // units: 0.01 °C
}

// ---------------------------------------------------------------------------
// Internal helper: compensate pressure (Bosch algorithm, 64-bit integer)
// Returns pressure in Pascals (e.g., 101325)
// ---------------------------------------------------------------------------
static int32_t compensate_pressure(const uint32_t raw_press)
{
    const int64_t t_lin = s_t_lin;

    int64_t partial_data1 = t_lin * t_lin;
    int64_t partial_data2 = (partial_data1 >> 6) *
                            (t_lin >> 8);
    int64_t partial_data3 = (partial_data2 *
                             static_cast<int64_t>(s_calib.par_p11)) >> 16;

    int64_t partial_data4 = (partial_data1 *
                             static_cast<int64_t>(s_calib.par_p10)) >> 17;

    partial_data3 = partial_data3 + partial_data4 +
                    ((t_lin * static_cast<int64_t>(s_calib.par_p9)) >> 16);

    // Partial output 1
    int64_t partial_out1 = (static_cast<int64_t>(s_calib.par_p8) * (1LL << 15)) +
                           (partial_data3 >> 15);

    // Partial output 2
    partial_data1 = (static_cast<int64_t>(s_calib.par_p7) *
                     static_cast<int64_t>(s_calib.par_p7)) >> 8;

    partial_data2 = (partial_data1 * t_lin * t_lin) >> 32;

    partial_data3 = (partial_data2 + (static_cast<int64_t>(s_calib.par_p6) * t_lin)) >> 17;

    partial_data1 = (static_cast<int64_t>(s_calib.par_p5) << 15) + partial_data3;

    // Final pressure calculation
    const int64_t raw = static_cast<int64_t>(raw_press);

    partial_data2 = (partial_out1 * raw) >> 14;
    partial_data3 = (partial_data1 * raw * raw) >> 32;

    int64_t partial_data4b = (static_cast<int64_t>(s_calib.par_p4) *
                              partial_data3) >> 3;

    int64_t partial_data5 = (static_cast<int64_t>(s_calib.par_p3) *
                             (raw * raw >> 8) >> 13);

    int64_t partial_data6 = (static_cast<int64_t>(s_calib.par_p2) - (1LL << 14)) *
                            raw >> 1;

    int64_t comp_press = (static_cast<int64_t>(s_calib.par_p1) - (1LL << 14)) * (1LL << 17) +
                         partial_data6 + partial_data5 + partial_data4b + partial_data2;

    comp_press = comp_press >> 8;

    // Scale to Pascals (units: Pa)
    return static_cast<int32_t>(comp_press / 100);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

Error init()
{
    // 1. Soft reset
    const uint8_t reset_cmd = kCmdSoftReset;
    Error err = hal::i2c_write_register(
        config::kBmp390I2cAddress, kRegCmd, &reset_cmd, 1);
    if (err != Error::kOk) {
        return err;
    }
    delay(kSoftResetDelayMs);

    // 2. Verify chip ID
    uint8_t chip_id = 0;
    err = hal::i2c_read_register(
        config::kBmp390I2cAddress, kRegChipId, &chip_id, 1);
    if (err != Error::kOk) {
        return err;
    }
    if (chip_id != kChipId) {
        return Error::kInvalidChipId;
    }

    // 3. Read calibration data (21 bytes from NVM)
    uint8_t calib_raw[kCalibDataLen] = {};
    err = hal::i2c_read_register(
        config::kBmp390I2cAddress, kRegNvmPar, calib_raw, kCalibDataLen);
    if (err != Error::kOk) {
        return err;
    }
    parse_calib_data(calib_raw);

    // 4. Configure oversampling (OSR register)
    const uint8_t osr_val = (config::kBmp390OsrTemperature << 3) |
                            config::kBmp390OsrPressure;
    err = hal::i2c_write_register(
        config::kBmp390I2cAddress, kRegOsr, &osr_val, 1);
    if (err != Error::kOk) {
        return err;
    }

    // 5. Configure output data rate (ODR register)
    err = hal::i2c_write_register(
        config::kBmp390I2cAddress, kRegOdr, &config::kBmp390Odr, 1);
    if (err != Error::kOk) {
        return err;
    }

    // 6. Configure IIR filter (CONFIG register)
    const uint8_t filter_val = config::kBmp390IirFilterCoeff << 1;
    err = hal::i2c_write_register(
        config::kBmp390I2cAddress, kRegConfig, &filter_val, 1);
    if (err != Error::kOk) {
        return err;
    }

    // 7. Enable pressure + temperature, normal mode
    const uint8_t pwr_ctrl = kPwrCtrlPressEn | kPwrCtrlTempEn |
                             kPwrCtrlModeNormal;
    err = hal::i2c_write_register(
        config::kBmp390I2cAddress, kRegPwrCtrl, &pwr_ctrl, 1);
    if (err != Error::kOk) {
        return err;
    }

    s_initialized = true;
    return Error::kOk;
}

Error read_pressure_temperature(PressureData* out)
{
    if (!s_initialized) {
        return Error::kNotInitialized;
    }
    if (out == nullptr) {
        return Error::kInvalidData;
    }

    // Read 6 bytes: P[XLSB,LSB,MSB] + T[XLSB,LSB,MSB]
    uint8_t data[6] = {};
    const Error err = hal::i2c_read_register(
        config::kBmp390I2cAddress, kRegData0, data, 6);
    if (err != Error::kOk) {
        return err;
    }

    // Assemble 24-bit raw values (little-endian)
    const uint32_t raw_press = static_cast<uint32_t>(data[2]) << 16 |
                               static_cast<uint32_t>(data[1]) << 8  |
                               static_cast<uint32_t>(data[0]);

    const uint32_t raw_temp  = static_cast<uint32_t>(data[5]) << 16 |
                               static_cast<uint32_t>(data[4]) << 8  |
                               static_cast<uint32_t>(data[3]);

    // Compensate (temperature must be computed first — sets s_t_lin)
    out->temperature_cdeg = compensate_temperature(raw_temp);
    out->pressure_pa      = compensate_pressure(raw_press);

    return Error::kOk;
}

}  // namespace bmp390
}  // namespace vario
