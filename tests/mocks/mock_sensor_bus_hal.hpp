#pragma once

#include "hal/sensor_bus_hal.hpp"
#include <cstdint>

namespace vario::hal {

struct MockSensorBusState {
    uint8_t registers[256];
    Error read_status;
    Error write_status;
    uint32_t read_count;
    uint32_t write_count;
};

extern MockSensorBusState g_mock_sensor_bus;

void mock_sensor_bus_reset();
void mock_sensor_bus_set_register(uint8_t reg, uint8_t val);
void mock_sensor_bus_set_registers(uint8_t start_reg, const uint8_t* data, uint8_t len);

}  // namespace vario::hal

