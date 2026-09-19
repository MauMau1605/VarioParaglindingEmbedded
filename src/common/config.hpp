/// @file config.hpp
/// @brief Project-wide compile-time configuration constants.

#ifndef VARIOUSB_COMMON_CONFIG_HPP
#define VARIOUSB_COMMON_CONFIG_HPP

#include <cstdint>

namespace vario {
namespace config {

// ---------------------------------------------------------------------------
// Sensor Bus Interface Selection (0 = I2C, 1 = SPI)
// ---------------------------------------------------------------------------

#ifndef VARIO_USE_SPI
/// Set to 1 to communicate with BMP390 via SPI (4-wire), 0 for I2C.
#define VARIO_USE_SPI 0
#endif

// ---------------------------------------------------------------------------
// SPI Configuration (active when VARIO_USE_SPI == 1)
// ---------------------------------------------------------------------------

/// Chip Select (CS) pin for BMP390 (Seeed XIAO D3 / Physical Pin 4)
static constexpr uint8_t kSpiCsPin = 3;

// ---------------------------------------------------------------------------
// I2C Configuration (active when VARIO_USE_SPI == 0)
// ---------------------------------------------------------------------------

/// BMP390 I2C address (SDO wired to 3.3V = HIGH → address 0x77, GND → 0x76)
static constexpr uint8_t kBmp390I2cAddress = 0x77;

// ---------------------------------------------------------------------------
// Timing Configuration
// ---------------------------------------------------------------------------

/// Pressure acquisition interval in milliseconds (50 Hz)
static constexpr uint32_t kAcquisitionIntervalMs = 20;

/// LK8EX1 sentence transmission interval in milliseconds (10 Hz)
static constexpr uint32_t kTransmitIntervalMs = 100;

// ---------------------------------------------------------------------------
// USB Serial Configuration
// ---------------------------------------------------------------------------

/// USB CDC baud rate (standard for variometer protocols)
static constexpr uint32_t kUsbBaudRate = 115200;

/// Maximum time to wait for USB connection at startup (ms)
static constexpr uint32_t kUsbInitTimeoutMs = 2000;

// ---------------------------------------------------------------------------
// BMP390 Sensor Configuration
// ---------------------------------------------------------------------------

/// Pressure oversampling (x8 recommended for variometer use)
static constexpr uint8_t kBmp390OsrPressure = 3;  // 0=x1, 1=x2, 2=x4, 3=x8, 4=x16, 5=x32

/// Temperature oversampling (x1 sufficient for variometer)
static constexpr uint8_t kBmp390OsrTemperature = 0;  // 0=x1

/// IIR filter coefficient (coeff 3 for moderate smoothing)
static constexpr uint8_t kBmp390IirFilterCoeff = 2;  // 0=off, 1=1, 2=3, 3=7, 4=15, 5=31, 6=63, 7=127

/// Output data rate setting (50 Hz)
static constexpr uint8_t kBmp390Odr = 0x02;  // 0x00=200Hz, 0x01=100Hz, 0x02=50Hz, 0x03=25Hz

// ---------------------------------------------------------------------------
// LK8EX1 Formatter Configuration
// ---------------------------------------------------------------------------

/// Maximum buffer size for a single LK8EX1 sentence
static constexpr uint8_t kLk8ex1MaxLen = 64;

/// Altitude placeholder (unknown, to be computed by phone app)
static constexpr int32_t kAltitudeUnknown = 99999;

/// Battery placeholder (not managed on this hardware)
static constexpr int16_t kBatteryUnknown = 999;

// ---------------------------------------------------------------------------
// Vario Engine Configuration
// ---------------------------------------------------------------------------

/// IIR filter alpha coefficient (0–256 range, 256 = no filtering)
/// At 50 Hz (dt = 20 ms), alpha = 26/256 ≈ 0.10 gives tau ≈ 187 ms time constant,
/// providing fast thermal entry response while damping high-frequency pressure jitter.
static constexpr uint16_t kVarioFilterAlpha = 26;

/// Scale denominator for the alpha filter (fixed-point arithmetic)
static constexpr uint16_t kVarioFilterScale = 256;

}  // namespace config
}  // namespace vario

#endif  // VARIOUSB_COMMON_CONFIG_HPP
