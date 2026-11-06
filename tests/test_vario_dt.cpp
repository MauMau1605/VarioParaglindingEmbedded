#include "check.hpp"
#include "common/baro_math.hpp"
#include <iostream>

using namespace vario::baro_math;

int main() {
    std::cout << "Running test_vario_dt..." << std::endl;

    // elapsed 19/20/21 ms -> dt = 20
    CHECK(quantize_baro_dt_ms(19) == 20);
    CHECK(quantize_baro_dt_ms(20) == 20);
    CHECK(quantize_baro_dt_ms(21) == 20);

    // 38/41 ms -> dt = 40
    CHECK(quantize_baro_dt_ms(38) == 40);
    CHECK(quantize_baro_dt_ms(41) == 40);

    // 0-9 ms -> dt = 20 (never 0)
    CHECK(quantize_baro_dt_ms(0) == 20);
    CHECK(quantize_baro_dt_ms(5) == 20);
    CHECK(quantize_baro_dt_ms(9) == 20);

    if (g_test_failures == 0) {
        std::cout << "All test_vario_dt tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_vario_dt!" << std::endl;
        return 1;
    }
}

