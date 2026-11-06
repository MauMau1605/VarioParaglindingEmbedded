/// @file config.hpp
/// @brief Project-wide compile-time configuration constants for VarioGPS-BLE.

#ifndef VARIO_COMMON_CONFIG_HPP
#define VARIO_COMMON_CONFIG_HPP

#include <cstdint>

namespace vario {
namespace config {

// ---------------------------------------------------------------------------
// Barometer I2C & Timing Configuration
// ---------------------------------------------------------------------------

/// Main loop timer tick (ms)
static constexpr uint32_t kMainTickMs = 10;

/// BMP390 nominal sample period (50 Hz)
static constexpr uint32_t kBaroSamplePeriodMs = 20;

/// Buzzer update tick count (update every 5 * 10 ms = 50 ms)
static constexpr uint32_t kBuzzerUpdateTicks = 5;

/// Watchdog timeout in ms
static constexpr uint32_t kWdtTimeoutMs = 2000;

/// BMP390 I2C address (SDO tied to GND on custom PCB -> 0x76)
static constexpr uint8_t kBmp390I2cAddress = 0x76;

/// Pressure acquisition interval in milliseconds (50 Hz)
static constexpr uint32_t kAcquisitionIntervalMs = 20;

/// Default LK8EX1 transmission interval in milliseconds (10 Hz)
static constexpr uint32_t kTransmitIntervalMs = 100;

/// GNSS update interval in milliseconds (1 Hz)
static constexpr uint32_t kGpsIntervalMs = 1000;

/// Battery voltage check interval in milliseconds (30 seconds)
static constexpr uint32_t kBatteryIntervalMs = 30000;

// ---------------------------------------------------------------------------
// BMP390 Sensor Configuration
// ---------------------------------------------------------------------------

/// Pressure oversampling (x8 recommended for variometer use)
static constexpr uint8_t kBmp390OsrPressure = 3;

/// Temperature oversampling (x1 sufficient for variometer)
static constexpr uint8_t kBmp390OsrTemperature = 0;

/// On-chip IIR filter coefficient (coeff 3 for moderate smoothing)
static constexpr uint8_t kBmp390IirFilterCoeff = 2;

/// Output data rate setting (50 Hz)
static constexpr uint8_t kBmp390Odr = 0x02;

// ---------------------------------------------------------------------------
// LK8EX1 Formatter Configuration
// ---------------------------------------------------------------------------

/// Maximum buffer size for a single LK8EX1 sentence
static constexpr uint8_t kLk8ex1MaxLen = 64;

/// Altitude placeholder when baro altitude is not available
static constexpr int32_t kAltitudeUnknown = 99999;

/// Battery placeholder when battery status is unknown
static constexpr int16_t kBatteryUnknown = 999;

// ---------------------------------------------------------------------------
// Vario Engine Configuration
// ---------------------------------------------------------------------------

/// IIR filter alpha coefficient (26/256 ≈ 0.10, tau ≈ 187 ms at 50 Hz)
static constexpr uint16_t kVarioFilterAlpha = 26;

/// Scale denominator for alpha filter (fixed-point arithmetic)
static constexpr uint16_t kVarioFilterScale = 256;

/// Barometric scale numerator for cPa -> cm/s conversion (-0.0832 m/Pa)
static constexpr int32_t kBaroScaleNum = -832;

/// Barometric scale denominator
static constexpr int32_t kBaroScaleDenom = 10000;

// ---------------------------------------------------------------------------
// GNSS SAM-M10Q Configuration
// ---------------------------------------------------------------------------

/// Maximum initialization attempts
static constexpr uint8_t kGpsInitMaxAttempts = 2;

/// Configuration packet sends per baud rate test
static constexpr uint8_t kGpsConfigSendsPerBaud = 2;

/// Timeout waiting for UBX ACK in ms
static constexpr uint32_t kGpsAckTimeoutMs = 250;

/// GPS baud rate for cold boot handshake
static constexpr uint32_t kGpsBaudCold = 9600;

/// GPS baud rate for normal run operation
static constexpr uint32_t kGpsBaudRun = 115200;

/// Factory default boot baud rate for SAM-M10Q
static constexpr uint32_t kGpsBootBaudRate = 9600;

/// Operational high-speed baud rate for UBX communication
static constexpr uint32_t kGpsOperatingBaudRate = 115200;

/// Expected UBX-NAV-PVT payload length
static constexpr uint16_t kUbxNavPvtPayloadLen = 92;

/// Total UBX-NAV-PVT frame length (sync + header + payload + checksum)
static constexpr uint16_t kUbxNavPvtFrameLen = 100;

/// UART RX inactivity timeout in microseconds for EasyDMA end-of-frame
static constexpr uint32_t kUartRxTimeoutUs = 5000;

// ---------------------------------------------------------------------------
// QNH Calibration & Takeoff Detection Configuration
// ---------------------------------------------------------------------------

/// 1 Hz samples in moving average window for P0 calculation
static constexpr uint8_t kQnhWindowSamples = 20;

/// Minimum samples required before initial P0 calculation
static constexpr uint8_t kQnhMinSamples = 5;

/// Update P0 only below this ground speed (mm/s)
static constexpr int32_t kQnhStationarySpeedMmS = 1500;

/// Takeoff vertical speed trigger (|Vz| > 1.2 m/s) in cm/s
static constexpr int32_t kTakeoffVzCmS = 120;

/// Takeoff vertical speed trigger with ground speed (|Vz| > 0.5 m/s) in cm/s
static constexpr int32_t kTakeoffVzWithSpeedCmS = 50;

/// Takeoff ground speed trigger (2.5 m/s) in mm/s
static constexpr int32_t kTakeoffSpeedMmS = 2500;

/// Takeoff condition hold duration in ms (3 s)
static constexpr uint32_t kTakeoffHoldMs = 3000;

/// Standard ISA sea-level pressure in Pascals
static constexpr int32_t kIsaSeaLevelPa = 101325;

// ---------------------------------------------------------------------------
// Buzzer / Audio Configuration
// ---------------------------------------------------------------------------

/// Minimum climb rate to start beeping (cm/s, deadband upper boundary)
static constexpr int32_t kBuzzerClimbMinCms = 30;

/// Maximum climb rate for audio frequency saturation (cm/s)
static constexpr int32_t kBuzzerClimbMaxCms = 500;

/// Sink alarm threshold (cm/s, deadband lower boundary)
static constexpr int32_t kBuzzerSinkThresholdCms = -100;

/// Buzzer base frequency at minimum climb rate (Hz)
static constexpr uint32_t kBuzzerFreqClimbMinHz = 700;

/// Buzzer max frequency at maximum climb rate (Hz)
static constexpr uint32_t kBuzzerFreqClimbMaxHz = 2500;

/// Beep period at minimum climb rate (ms)
static constexpr uint32_t kBuzzerPeriodClimbMaxMs = 600;

/// Beep period at maximum climb rate (ms)
static constexpr uint32_t kBuzzerPeriodClimbMinMs = 150;

/// Continuous sink tone frequency (Hz)
static constexpr uint32_t kBuzzerFreqSinkHz = 300;

// ---------------------------------------------------------------------------
// Battery & ADC Configuration
// ---------------------------------------------------------------------------

/// Battery sampling interval in ms (30 seconds)
static constexpr uint32_t kBatterySampleIntervalMs = 30000;

/// Battery calibration multiplier numerator
static constexpr int32_t kBatteryCalNum = 1;

/// Battery calibration multiplier denominator
static constexpr int32_t kBatteryCalDen = 1;

/// Number of ADC readings averaged per battery update
static constexpr uint8_t kAdcAverageSamples = 8;

/// Minimum LiPo battery voltage in millivolts (0% SoC)
static constexpr uint16_t kBatteryCutoffMv = 3000;

/// Maximum LiPo battery voltage in millivolts (100% SoC)
static constexpr uint16_t kBatteryFullMv = 4200;

}  // namespace config
}  // namespace vario

#endif  // VARIO_COMMON_CONFIG_HPP
