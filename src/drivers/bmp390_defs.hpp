/// @file bmp390_defs.hpp
/// @brief BMP390 register addresses, constants, and calibration structures.
///        Based on Bosch BST-BMP390-DS002 datasheet.
///        Register map is compatible with BMP388.

#ifndef VARIOUSB_DRIVERS_BMP390_DEFS_HPP
#define VARIOUSB_DRIVERS_BMP390_DEFS_HPP

#include <cstdint>

namespace vario {
namespace bmp390 {

// ---------------------------------------------------------------------------
// Chip identification
// ---------------------------------------------------------------------------
static constexpr uint8_t kChipId       = 0x60;  // BMP390
static constexpr uint8_t kChipIdBmp388 = 0x50;  // BMP388 (identical register map & compensation)

// ---------------------------------------------------------------------------
// Register addresses
// ---------------------------------------------------------------------------
static constexpr uint8_t kRegChipId    = 0x00;
static constexpr uint8_t kRegErrReg    = 0x02;
static constexpr uint8_t kRegStatus    = 0x03;
static constexpr uint8_t kRegData0     = 0x04;  // Pressure XLSB
static constexpr uint8_t kRegData1     = 0x05;  // Pressure LSB
static constexpr uint8_t kRegData2     = 0x06;  // Pressure MSB
static constexpr uint8_t kRegData3     = 0x07;  // Temperature XLSB
static constexpr uint8_t kRegData4     = 0x08;  // Temperature LSB
static constexpr uint8_t kRegData5     = 0x09;  // Temperature MSB
static constexpr uint8_t kRegIntCtrl   = 0x19;
static constexpr uint8_t kRegIfConf    = 0x1A;
static constexpr uint8_t kRegPwrCtrl   = 0x1B;
static constexpr uint8_t kRegOsr       = 0x1C;
static constexpr uint8_t kRegOdr       = 0x1D;
static constexpr uint8_t kRegConfig    = 0x1F;
static constexpr uint8_t kRegCmd       = 0x7E;

// ---------------------------------------------------------------------------
// NVM calibration data registers (trimming parameters)
// ---------------------------------------------------------------------------
static constexpr uint8_t kRegNvmPar    = 0x31;  // Start of 21-byte calibration block
static constexpr uint8_t kCalibDataLen = 21;

// ---------------------------------------------------------------------------
// PWR_CTRL register bits
// ---------------------------------------------------------------------------
static constexpr uint8_t kPwrCtrlPressEn = 0x01;  // Enable pressure measurement
static constexpr uint8_t kPwrCtrlTempEn  = 0x02;  // Enable temperature measurement
static constexpr uint8_t kPwrCtrlModeNormal = 0x30;  // Normal (continuous) mode

// ---------------------------------------------------------------------------
// CMD register values
// ---------------------------------------------------------------------------
static constexpr uint8_t kCmdSoftReset = 0xB6;

// ---------------------------------------------------------------------------
// Status register bits
// ---------------------------------------------------------------------------
static constexpr uint8_t kStatusCmdRdy     = 0x10;
static constexpr uint8_t kStatusDrdy_Press  = 0x20;
static constexpr uint8_t kStatusDrdy_Temp   = 0x40;

// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------
static constexpr uint16_t kSoftResetDelayMs = 10;   // Wait after soft reset

// ---------------------------------------------------------------------------
// Calibration data structure (from NVM trimming parameters)
// ---------------------------------------------------------------------------
struct CalibData {
    // Temperature compensation coefficients
    uint16_t par_t1;
    uint16_t par_t2;
    int8_t   par_t3;

    // Pressure compensation coefficients
    int16_t  par_p1;
    int16_t  par_p2;
    int8_t   par_p3;
    int8_t   par_p4;
    uint16_t par_p5;
    uint16_t par_p6;
    int8_t   par_p7;
    int8_t   par_p8;
    int16_t  par_p9;
    int8_t   par_p10;
    int8_t   par_p11;
};

// ---------------------------------------------------------------------------
// Sensor limits (Bosch BST-BMP390-DS002 / bmp3_defs.h)
// ---------------------------------------------------------------------------
static constexpr uint32_t kMinPressureCpa = 3000000;   // 300.00 hPa (30 000 Pa)
static constexpr uint32_t kMaxPressureCpa = 12500000;  // 1250.00 hPa (125 000 Pa)
static constexpr int32_t  kMinTempCdeg    = -4000;     // -40.00 °C
static constexpr int32_t  kMaxTempCdeg    = 8500;      // +85.00 °C

}  // namespace bmp390
}  // namespace vario

#endif  // VARIOUSB_DRIVERS_BMP390_DEFS_HPP
