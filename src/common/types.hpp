/// @file types.hpp
/// @brief Shared data types used across all layers of VarioGPS-BLE.

#ifndef VARIO_COMMON_TYPES_HPP
#define VARIO_COMMON_TYPES_HPP

#include <cstdint>

namespace vario {

/// @brief Compensated pressure and temperature from the BMP390 driver.
struct PressureData {
    int32_t pressure_pa;       ///< Pressure in whole Pascals
    int32_t pressure_cpa;      ///< Pressure in centiPascals (0.01 Pa resolution)
    int32_t temperature_cdeg;  ///< Temperature in centidegrees Celsius (0.01 °C)
};

/// @brief Processed variometer telemetry data.
struct VarioData {
    int32_t pressure_pa;       ///< Pressure in whole Pascals
    int32_t vario_cm_s;        ///< Vertical speed in cm/s (+ climb, - sink)
    int32_t alt_baro_cm;       ///< Barometric altitude in cm MSL
    int16_t temperature_dc;    ///< Temperature in decidegrees Celsius (0.1 °C)
};

/// @brief Normalized GNSS fix data extracted from UBX-NAV-PVT.
struct GnssFix {
    bool     valid;              ///< Passes fix quality gates
    int32_t  latitude_deg_e7;    ///< Latitude in degrees * 1e7
    int32_t  longitude_deg_e7;   ///< Longitude in degrees * 1e7
    int32_t  altitude_msl_mm;    ///< Height above Mean Sea Level in mm
    int32_t  ground_speed_mm_s;  ///< 2D ground speed in mm/s
    int32_t  heading_deg_e5;     ///< Heading of motion in degrees * 1e5
    uint32_t v_acc_mm;           ///< Vertical accuracy estimate in mm
    uint8_t  satellites;         ///< Number of satellites used in Nav solution
    uint8_t  fix_type;           ///< 0=No fix, 2=2D, 3=3D
    uint8_t  hour;               ///< UTC hour (0-23)
    uint8_t  min;                ///< UTC minute (0-59)
    uint8_t  sec;                ///< UTC second (0-60)
    uint16_t year;               ///< UTC year
    uint8_t  month;              ///< UTC month (1-12)
    uint8_t  day;                ///< UTC day (1-31)
    uint32_t timestamp_ms;       ///< Local system uptime in ms
};

/// @brief Battery state of charge and voltage status.
struct BatteryStatus {
    uint16_t voltage_mv;         ///< Battery voltage in millivolts
    uint8_t  percent;            ///< Estimated state of charge (0-100%)
};

}  // namespace vario

#endif  // VARIO_COMMON_TYPES_HPP
