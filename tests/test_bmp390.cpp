#include "check.hpp"
#include "drivers/bmp390.hpp"
#include "drivers/bmp390_defs.hpp"
#include "mocks/mock_sensor_bus_hal.hpp"
#include "zephyr/kernel.h"
#include <iostream>

using namespace vario;
using namespace vario::bmp390;

static const uint8_t kMockCalibData[21] = {
    // par_t1: 27800 (0x6C98)
    0x98, 0x6C,
    // par_t2: 19500 (0x4C2C)
    0x2C, 0x4C,
    // par_t3: -10
    0xF6,
    // par_p1: 6500 (0x1964)
    0x64, 0x19,
    // par_p2: -4500 (0xEE6C)
    0x6C, 0xEE,
    // par_p3: 15
    0x0F,
    // par_p4: 8
    0x08,
    // par_p5: 21000 (0x5208)
    0x08, 0x52,
    // par_p6: 18000 (0x4650)
    0x50, 0x46,
    // par_p7: 5
    0x05,
    // par_p8: -3
    0xFD,
    // par_p9: 2500 (0x09C4)
    0xC4, 0x09,
    // par_p10: 12
    0x0C,
    // par_p11: -8
    0xF8
};

static void setup_mock_sensor(uint8_t chip_id) {
    hal::mock_sensor_bus_reset();
    hal::mock_sensor_bus_set_register(kRegChipId, chip_id);
    hal::mock_sensor_bus_set_registers(kRegNvmPar, kMockCalibData, sizeof(kMockCalibData));
    // Set DRDY bit in status reg
    hal::mock_sensor_bus_set_register(kRegStatus, kStatusDrdy_Press | kStatusDrdy_Temp);
}

static void test_chip_id_validation() {
    // Invalid ID -> rejected
    setup_mock_sensor(0x00);
    CHECK(init() == Error::kInvalidChipId);

    // BMP388 ID (0x50) -> accepted
    setup_mock_sensor(0x50);
    CHECK(init() == Error::kOk);
    CHECK_EQ(get_detected_chip_id(), 0x50);

    // BMP390 ID (0x60) -> accepted
    setup_mock_sensor(0x60);
    CHECK(init() == Error::kOk);
    CHECK_EQ(get_detected_chip_id(), 0x60);
}

static void test_compensation_math() {
    setup_mock_sensor(0x60);
    CHECK(init() == Error::kOk);

    // Provide raw sensor data:
    // 24-bit raw pressure: ~0x7E0000 -> bytes: 0x00, 0x00, 0x7E
    // 24-bit raw temp: ~0x800000 -> bytes: 0x00, 0x00, 0x80
    uint8_t raw_data[6] = {0x00, 0x40, 0x6E, 0x00, 0x20, 0x7E};
    hal::mock_sensor_bus_set_registers(kRegData0, raw_data, 6);

    PressureData out = {};
    Error err = read_pressure_temperature(&out);
    CHECK(err == Error::kOk);

    // Check reasonable ranges for pressure and temperature
    // Temperature: between -40 deg C and +85 deg C
    CHECK(out.temperature_cdeg >= kMinTempCdeg);
    CHECK(out.temperature_cdeg <= kMaxTempCdeg);

    // Pressure: between 300 hPa and 1250 hPa
    CHECK(out.pressure_pa >= 30000);
    CHECK(out.pressure_pa <= 125000);
    CHECK_EQ(out.pressure_pa, out.pressure_cpa / 100);
}

static void test_drdy_busy() {
    setup_mock_sensor(0x60);
    init();

    // Clear DRDY bit in status
    hal::mock_sensor_bus_set_register(kRegStatus, 0x00);

    PressureData out = {};
    Error err = read_pressure_temperature(&out);
    CHECK(err == Error::kBusy);
}

int main() {
    std::cout << "Running test_bmp390..." << std::endl;

    test_chip_id_validation();
    test_compensation_math();
    test_drdy_busy();

    if (g_test_failures == 0) {
        std::cout << "All test_bmp390 tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_bmp390!" << std::endl;
        return 1;
    }
}

