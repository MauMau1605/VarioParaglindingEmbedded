#include "tests/mocks/mock_sensor_bus_hal.hpp"
#include <cstring>

namespace vario::hal {

MockSensorBusState g_mock_sensor_bus = {{0}, Error::kOk, Error::kOk, 0, 0};

void mock_sensor_bus_reset() {
    std::memset(g_mock_sensor_bus.registers, 0, sizeof(g_mock_sensor_bus.registers));
    g_mock_sensor_bus.read_status = Error::kOk;
    g_mock_sensor_bus.write_status = Error::kOk;
    g_mock_sensor_bus.read_count = 0;
    g_mock_sensor_bus.write_count = 0;
}

void mock_sensor_bus_set_register(uint8_t reg, uint8_t val) {
    g_mock_sensor_bus.registers[reg] = val;
}

void mock_sensor_bus_set_registers(uint8_t start_reg, const uint8_t* data, uint8_t len) {
    if (data == nullptr) return;
    for (uint8_t i = 0; i < len; ++i) {
        g_mock_sensor_bus.registers[static_cast<uint8_t>(start_reg + i)] = data[i];
    }
}

Error sensor_bus_init() {
    return Error::kOk;
}

Error sensor_bus_read_register(uint8_t reg_addr, uint8_t* buffer, uint8_t len) {
    ++g_mock_sensor_bus.read_count;
    if (g_mock_sensor_bus.read_status != Error::kOk) {
        return g_mock_sensor_bus.read_status;
    }
    if (buffer == nullptr) return Error::kInvalidData;
    for (uint8_t i = 0; i < len; ++i) {
        buffer[i] = g_mock_sensor_bus.registers[static_cast<uint8_t>(reg_addr + i)];
    }
    return Error::kOk;
}

Error sensor_bus_write_register(uint8_t reg_addr, const uint8_t* data, uint8_t len) {
    ++g_mock_sensor_bus.write_count;
    if (g_mock_sensor_bus.write_status != Error::kOk) {
        return g_mock_sensor_bus.write_status;
    }
    if (data == nullptr) return Error::kInvalidData;
    for (uint8_t i = 0; i < len; ++i) {
        g_mock_sensor_bus.registers[static_cast<uint8_t>(reg_addr + i)] = data[i];
    }
    return Error::kOk;
}

}  // namespace vario::hal

