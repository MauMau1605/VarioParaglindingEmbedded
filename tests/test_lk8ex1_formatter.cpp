#include "check.hpp"
#include "middleware/lk8ex1_formatter.hpp"
#include <cstring>
#include <iostream>

using namespace vario;
using namespace vario::middleware;

static uint8_t compute_nmea_checksum(const char* sentence) {
    uint8_t cs = 0;
    bool counting = false;
    for (size_t i = 0; sentence[i] != '\0'; ++i) {
        if (sentence[i] == '$') {
            counting = true;
            continue;
        }
        if (sentence[i] == '*') {
            break;
        }
        if (counting) {
            cs ^= static_cast<uint8_t>(sentence[i]);
        }
    }
    return cs;
}

static void test_nominal_formatting() {
    VarioData data = {};
    data.pressure_pa = 101325;
    data.vario_cm_s = 150;
    data.temperature_dc = 215;  // 21.5 deg C -> 21
    data.alt_baro_cm = 12000;

    char buf[64] = {};
    uint8_t out_len = 0;

    Error err = lk8ex1_format(&data, buf, sizeof(buf), &out_len);
    CHECK(err == Error::kOk);
    CHECK(out_len > 0);
    CHECK(out_len == static_cast<uint8_t>(std::strlen(buf)));

    // Starts with $LK8EX1 and ends with \r\n
    CHECK(std::strncmp(buf, "$LK8EX1,101325,99999,150,21,999,*", 34) == 0);
    CHECK(buf[out_len - 2] == '\r');
    CHECK(buf[out_len - 1] == '\n');

    // Verify NMEA XOR checksum
    uint8_t expected_cs = compute_nmea_checksum(buf);
    char cs_hex[3] = {};
    cs_hex[0] = buf[out_len - 4];
    cs_hex[1] = buf[out_len - 3];
    uint8_t actual_cs = static_cast<uint8_t>(std::strtoul(cs_hex, nullptr, 16));
    CHECK_EQ(actual_cs, expected_cs);
}

static void test_sink_and_negative_temperature() {
    VarioData data = {};
    data.pressure_pa = 95400;
    data.vario_cm_s = -320;     // -3.20 m/s sink
    data.temperature_dc = -85;  // -8.5 deg C -> -8
    data.alt_baro_cm = 50000;

    char buf[64] = {};
    uint8_t out_len = 0;

    Error err = lk8ex1_format(&data, buf, sizeof(buf), &out_len);
    CHECK(err == Error::kOk);
    CHECK(std::strncmp(buf, "$LK8EX1,95400,99999,-320,-8,999,*", 34) == 0);

    uint8_t expected_cs = compute_nmea_checksum(buf);
    char cs_hex[3] = {buf[out_len - 4], buf[out_len - 3], '\0'};
    uint8_t actual_cs = static_cast<uint8_t>(std::strtoul(cs_hex, nullptr, 16));
    CHECK_EQ(actual_cs, expected_cs);
}

static void test_zero_values() {
    VarioData data = {};
    data.pressure_pa = 100000;
    data.vario_cm_s = 0;
    data.temperature_dc = 0;

    char buf[64] = {};
    uint8_t out_len = 0;

    Error err = lk8ex1_format(&data, buf, sizeof(buf), &out_len);
    CHECK(err == Error::kOk);
    CHECK(std::strncmp(buf, "$LK8EX1,100000,99999,0,0,999,*", 31) == 0);
}

static void test_buffer_boundary_checks() {
    VarioData data = {};
    data.pressure_pa = 101325;
    data.vario_cm_s = 150;
    data.temperature_dc = 200;

    char buf[64] = {};
    uint8_t out_len = 0;

    // Buffer < 40 must return Error::kInvalidData
    CHECK(lk8ex1_format(&data, buf, 39, &out_len) == Error::kInvalidData);
    CHECK(lk8ex1_format(&data, buf, 10, &out_len) == Error::kInvalidData);
    CHECK(lk8ex1_format(&data, buf, 0, &out_len) == Error::kInvalidData);

    // Buffer = 40 should succeed for standard sentences
    CHECK(lk8ex1_format(&data, buf, 40, &out_len) == Error::kOk);
    CHECK(out_len < 40);
}

static void test_null_pointer_checks() {
    VarioData data = {};
    char buf[64] = {};
    uint8_t out_len = 0;

    CHECK(lk8ex1_format(nullptr, buf, sizeof(buf), &out_len) == Error::kInvalidData);
    CHECK(lk8ex1_format(&data, nullptr, sizeof(buf), &out_len) == Error::kInvalidData);
    CHECK(lk8ex1_format(&data, buf, sizeof(buf), nullptr) == Error::kInvalidData);
}

int main() {
    std::cout << "Running test_lk8ex1_formatter..." << std::endl;

    test_nominal_formatting();
    test_sink_and_negative_temperature();
    test_zero_values();
    test_buffer_boundary_checks();
    test_null_pointer_checks();

    if (g_test_failures == 0) {
        std::cout << "All test_lk8ex1_formatter tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_lk8ex1_formatter!" << std::endl;
        return 1;
    }
}

