/// @file bmp390.cpp
/// @brief BMP390 driver implementation — init, calibration, compensated readout.
///        Compensation algorithm based on Bosch BMP3-Sensor-API (integer math only).
///        Reference: github.com/BoschSensortec/BMP3-Sensor-API  v2.0.6

#include "drivers/bmp390.hpp"
#include "drivers/bmp390_defs.hpp"
#include "hal/sensor_bus_hal.hpp"
#include "common/config.hpp"

#include <zephyr/kernel.h>

namespace vario {
namespace bmp390 {

// ---------------------------------------------------------------------------
// Module-level static state (no heap, no globals outside this TU)
// ---------------------------------------------------------------------------
static CalibData s_calib = {};
static int64_t   s_t_lin = 0;  // Linearized temperature for pressure compensation
static bool      s_initialized = false;
static uint8_t   s_detected_chip_id = 0;

// ---------------------------------------------------------------------------
// Internal helper: parse calibration NVM bytes into CalibData
// Byte order matches Bosch BMP3-Sensor-API parse_calib_data().
// ---------------------------------------------------------------------------
static void parse_calib_data(const uint8_t* raw)
{
    s_calib.par_t1  = static_cast<uint16_t>((static_cast<uint16_t>(raw[1]) << 8) | raw[0]);
    s_calib.par_t2  = static_cast<uint16_t>((static_cast<uint16_t>(raw[3]) << 8) | raw[2]);
    s_calib.par_t3  = static_cast<int8_t>(raw[4]);

    s_calib.par_p1  = static_cast<int16_t>((static_cast<uint16_t>(raw[6]) << 8) | raw[5]);
    s_calib.par_p2  = static_cast<int16_t>((static_cast<uint16_t>(raw[8]) << 8) | raw[7]);
    s_calib.par_p3  = static_cast<int8_t>(raw[9]);
    s_calib.par_p4  = static_cast<int8_t>(raw[10]);
    s_calib.par_p5  = static_cast<uint16_t>((static_cast<uint16_t>(raw[12]) << 8) | raw[11]);
    s_calib.par_p6  = static_cast<uint16_t>((static_cast<uint16_t>(raw[14]) << 8) | raw[13]);
    s_calib.par_p7  = static_cast<int8_t>(raw[15]);
    s_calib.par_p8  = static_cast<int8_t>(raw[16]);
    s_calib.par_p9  = static_cast<int16_t>((static_cast<uint16_t>(raw[18]) << 8) | raw[17]);
    s_calib.par_p10 = static_cast<int8_t>(raw[19]);
    s_calib.par_p11 = static_cast<int8_t>(raw[20]);
}

// ---------------------------------------------------------------------------
// Internal helper: compensate temperature — Bosch integer algorithm.
// Updates s_t_lin (needed by pressure compensation).
// Returns temperature in centidegrees Celsius (e.g., 2426 = 24.26 °C).
// Source: BMP3-Sensor-API bmp3.c  compensate_temperature() (integer version)
// ---------------------------------------------------------------------------
static int32_t compensate_temperature(const uint32_t raw_temp)
{
    const int64_t pd1 = static_cast<int64_t>(raw_temp) -
                        (static_cast<int64_t>(256) * s_calib.par_t1);
    const int64_t pd2 = static_cast<int64_t>(s_calib.par_t2) * pd1;
    const int64_t pd3 = pd1 * pd1;
    const int64_t pd4 = pd3 * static_cast<int64_t>(s_calib.par_t3);
    const int64_t pd5 = (pd2 * INT64_C(262144)) + pd4;

    // t_lin: linearised temperature used by pressure compensation
    s_t_lin = pd5 / INT64_C(4294967296);

    // comp_temp in 0.01 °C (100 = 1.00 °C, 2426 = 24.26 °C)
    int64_t comp_temp = (s_t_lin * INT64_C(25)) / INT64_C(16384);
    if (comp_temp < kMinTempCdeg) {
        comp_temp = kMinTempCdeg;
    } else if (comp_temp > kMaxTempCdeg) {
        comp_temp = kMaxTempCdeg;
    }
    return static_cast<int32_t>(comp_temp);
}

// ---------------------------------------------------------------------------
// Internal helper: compute offset and sensitivity terms for pressure.
// Separated to keep each function < 25 lines per coding standard.
// ---------------------------------------------------------------------------
static void calc_pressure_offset(int64_t* offset)
{
    const int64_t t = s_t_lin;
    const int64_t pd1 = t * t;
    const int64_t pd2 = pd1 / INT64_C(64);
    const int64_t pd3 = (pd2 * t) / INT64_C(256);
    const int64_t pd4 = (static_cast<int64_t>(s_calib.par_p8) * pd3) / INT64_C(32);
    const int64_t pd5 = static_cast<int64_t>(s_calib.par_p7) * pd1 * INT64_C(16);
    const int64_t pd6 = static_cast<int64_t>(s_calib.par_p6) * t * INT64_C(4194304);
    *offset = static_cast<int64_t>(s_calib.par_p5) * INT64_C(140737488355328) +
              pd4 + pd5 + pd6;
}

static void calc_pressure_sensitivity(int64_t* sensitivity)
{
    const int64_t t = s_t_lin;
    const int64_t pd1 = t * t;
    const int64_t pd2 = (static_cast<int64_t>(s_calib.par_p4) *
                         ((pd1 / INT64_C(64)) * t / INT64_C(256))) / INT64_C(32);
    const int64_t pd4 = static_cast<int64_t>(s_calib.par_p3) * pd1 * INT64_C(4);
    const int64_t pd5 = (static_cast<int64_t>(s_calib.par_p2) - INT64_C(16384)) *
                         t * INT64_C(2097152);
    *sensitivity = (static_cast<int64_t>(s_calib.par_p1) - INT64_C(16384)) *
                   INT64_C(70368744177664) + pd2 + pd4 + pd5;
}

// ---------------------------------------------------------------------------
// Internal helper: compensate pressure — Bosch integer algorithm.
// Returns pressure in centiPascals (e.g., 10132500 = 101325.00 Pa = 1013.25 hPa).
// Source: BMP3-Sensor-API bmp3.c  compensate_pressure() (integer version)
// ---------------------------------------------------------------------------
static int64_t calc_pressure_terms(const int64_t raw)
{
    const int64_t pd2 = static_cast<int64_t>(s_calib.par_p10) * s_t_lin;
    const int64_t pd3 = pd2 + (INT64_C(65536) * static_cast<int64_t>(s_calib.par_p9));
    const int64_t pd4 = (pd3 * raw) / INT64_C(8192);
    const int64_t pd5 = (raw * (pd4 / INT64_C(10))) / INT64_C(512) * INT64_C(10);
    const int64_t pd6  = raw * raw;
    const int64_t pd7  = (static_cast<int64_t>(s_calib.par_p11) * pd6) / INT64_C(65536);
    const int64_t pd8  = (pd7 * raw) / INT64_C(128);
    return pd5 + pd8;
}

static int32_t compensate_pressure_cpa(const uint32_t raw_press)
{
    int64_t offset = 0;
    int64_t sensitivity = 0;
    calc_pressure_offset(&offset);
    calc_pressure_sensitivity(&sensitivity);

    const int64_t raw = static_cast<int64_t>(raw_press);
    const int64_t pd1 = (sensitivity / INT64_C(16777216)) * raw;
    const int64_t pd9 = (offset / INT64_C(4)) + pd1 + calc_pressure_terms(raw);

    if (pd9 <= 0) return static_cast<int32_t>(kMinPressureCpa);

    uint64_t comp_cpa = (static_cast<uint64_t>(pd9) * UINT64_C(25)) / UINT64_C(1099511627776);
    if (comp_cpa < kMinPressureCpa) comp_cpa = kMinPressureCpa;
    else if (comp_cpa > kMaxPressureCpa) comp_cpa = kMaxPressureCpa;
    return static_cast<int32_t>(comp_cpa);
}

static Error verify_chip_id()
{
    s_detected_chip_id = 0;
    const Error err = hal::sensor_bus_read_register(kRegChipId, &s_detected_chip_id, 1);
    if (err != Error::kOk) return err;
    if (s_detected_chip_id != kChipId && s_detected_chip_id != kChipIdBmp388) {
        return Error::kInvalidChipId;
    }
    return Error::kOk;
}

static Error soft_reset()
{
    const uint8_t reset_cmd = kCmdSoftReset;
    const Error err = hal::sensor_bus_write_register(kRegCmd, &reset_cmd, 1);
    if (err != Error::kOk) return err;
    k_msleep(kSoftResetDelayMs);
    return Error::kOk;
}

static Error load_calibration()
{
    uint8_t calib_raw[kCalibDataLen] = {};
    const Error err = hal::sensor_bus_read_register(kRegNvmPar, calib_raw, kCalibDataLen);
    if (err != Error::kOk) return err;
    parse_calib_data(calib_raw);
    return Error::kOk;
}

static Error configure_sensor()
{
    const uint8_t osr_val = (config::kBmp390OsrTemperature << 3) | config::kBmp390OsrPressure;
    if (hal::sensor_bus_write_register(kRegOsr, &osr_val, 1) != Error::kOk) return Error::kWriteFailed;
    if (hal::sensor_bus_write_register(kRegOdr, &config::kBmp390Odr, 1) != Error::kOk) return Error::kWriteFailed;
    const uint8_t filter_val = config::kBmp390IirFilterCoeff << 1;
    if (hal::sensor_bus_write_register(kRegConfig, &filter_val, 1) != Error::kOk) return Error::kWriteFailed;
    const uint8_t pwr_ctrl = kPwrCtrlPressEn | kPwrCtrlTempEn | kPwrCtrlModeNormal;
    if (hal::sensor_bus_write_register(kRegPwrCtrl, &pwr_ctrl, 1) != Error::kOk) return Error::kWriteFailed;
    return Error::kOk;
}

Error init()
{
    Error err = verify_chip_id();
    if (err != Error::kOk) return err;
    err = soft_reset();
    if (err != Error::kOk) return err;
    err = load_calibration();
    if (err != Error::kOk) return err;
    err = configure_sensor();
    if (err != Error::kOk) return err;
    s_initialized = true;
    return Error::kOk;
}

static void unpack_raw_sensor_data(const uint8_t* d, uint32_t* p, uint32_t* t)
{
    *p = static_cast<uint32_t>(d[2]) << 16 | static_cast<uint32_t>(d[1]) << 8 | static_cast<uint32_t>(d[0]);
    *t = static_cast<uint32_t>(d[5]) << 16 | static_cast<uint32_t>(d[4]) << 8 | static_cast<uint32_t>(d[3]);
}

Error read_pressure_temperature(PressureData* out)
{
    if (!s_initialized) return Error::kNotInitialized;
    if (out == nullptr) return Error::kInvalidData;

    uint8_t status = 0;
    Error err = hal::sensor_bus_read_register(kRegStatus, &status, 1);
    if (err != Error::kOk) return err;
    if ((status & kStatusDrdy_Press) == 0) {
        return Error::kBusy;
    }

    uint8_t data[6] = {};
    err = hal::sensor_bus_read_register(kRegData0, data, 6);
    if (err != Error::kOk) return err;

    uint32_t raw_press = 0;
    uint32_t raw_temp = 0;
    unpack_raw_sensor_data(data, &raw_press, &raw_temp);

    out->temperature_cdeg = compensate_temperature(raw_temp);
    out->pressure_cpa     = compensate_pressure_cpa(raw_press);
    out->pressure_pa      = out->pressure_cpa / 100;
    return Error::kOk;
}

uint8_t get_detected_chip_id()
{
    return s_detected_chip_id;
}

}  // namespace bmp390
}  // namespace vario
