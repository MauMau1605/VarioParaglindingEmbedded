#include "check.hpp"
#include "drivers/ubx_parser.hpp"
#include <vector>
#include <cstring>
#include <iostream>

using namespace vario::drivers;

static void compute_fletcher8(const uint8_t* data, size_t len, uint8_t& ck_a, uint8_t& ck_b) {
    ck_a = 0;
    ck_b = 0;
    for (size_t i = 0; i < len; ++i) {
        ck_a = static_cast<uint8_t>(ck_a + data[i]);
        ck_b = static_cast<uint8_t>(ck_b + ck_a);
    }
}

static std::vector<uint8_t> make_ubx_frame(uint8_t cls, uint8_t id, const uint8_t* payload, uint16_t len) {
    std::vector<uint8_t> frame;
    frame.push_back(0xB5);
    frame.push_back(0x62);
    frame.push_back(cls);
    frame.push_back(id);
    frame.push_back(static_cast<uint8_t>(len & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    if (len > 0 && payload != nullptr) {
        frame.insert(frame.end(), payload, payload + len);
    }
    uint8_t ck_a = 0, ck_b = 0;
    compute_fletcher8(frame.data() + 2, frame.size() - 2, ck_a, ck_b);
    frame.push_back(ck_a);
    frame.push_back(ck_b);
    return frame;
}

int main() {
    std::cout << "Running test_ubx_parser..." << std::endl;

    UbxParser parser;

    // Build a mock UbxNavPvt
    UbxNavPvt mock_pvt = {};
    mock_pvt.iTOW = 123456789;
    mock_pvt.year = 2026;
    mock_pvt.month = 11;
    mock_pvt.day = 6;
    mock_pvt.hour = 12;
    mock_pvt.min = 34;
    mock_pvt.sec = 56;
    mock_pvt.lat = 451234567;
    mock_pvt.lon = 567890123;
    mock_pvt.hMSL = 1250000;
    mock_pvt.gSpeed = 1500;
    mock_pvt.fixType = 3;
    mock_pvt.numSV = 12;

    auto pvt_frame = make_ubx_frame(0x01, 0x07, reinterpret_cast<const uint8_t*>(&mock_pvt), sizeof(mock_pvt));
    CHECK(pvt_frame.size() == 100);

    // 1. Valid NAV-PVT fed one byte at a time
    parser.reset();
    int pvt_count = 0;
    for (uint8_t b : pvt_frame) {
        if (parser.feed(b) == UbxEvent::kNavPvt) {
            ++pvt_count;
        }
    }
    CHECK(pvt_count == 1);
    CHECK(parser.nav_pvt().iTOW == mock_pvt.iTOW);
    CHECK(parser.nav_pvt().lat == mock_pvt.lat);
    CHECK(parser.nav_pvt().lon == mock_pvt.lon);
    CHECK(parser.nav_pvt().hMSL == mock_pvt.hMSL);

    // 2. Preceded by NMEA text and split in chunks
    parser.reset();
    std::vector<uint8_t> stream;
    const char* nmea = "$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
    stream.insert(stream.end(), nmea, nmea + strlen(nmea));
    stream.insert(stream.end(), pvt_frame.begin(), pvt_frame.end());

    pvt_count = 0;
    for (uint8_t b : stream) {
        if (parser.feed(b) == UbxEvent::kNavPvt) {
            ++pvt_count;
        }
    }
    CHECK(pvt_count == 1);

    // 3. Corrupt one payload byte -> 0 events; next valid frame is parsed
    parser.reset();
    auto corrupt_frame = pvt_frame;
    corrupt_frame[15] ^= 0xFF; // flip bits in payload
    pvt_count = 0;
    for (uint8_t b : corrupt_frame) {
        if (parser.feed(b) == UbxEvent::kNavPvt) {
            ++pvt_count;
        }
    }
    CHECK(pvt_count == 0);

    // Next valid frame must succeed (resync)
    for (uint8_t b : pvt_frame) {
        if (parser.feed(b) == UbxEvent::kNavPvt) {
            ++pvt_count;
        }
    }
    CHECK(pvt_count == 1);

    // 4. UBX-ACK-ACK and UBX-ACK-NAK
    parser.reset();
    uint8_t ack_payload[2] = {0x06, 0x8A};
    auto ack_frame = make_ubx_frame(0x05, 0x01, ack_payload, 2);
    int ack_count = 0;
    for (uint8_t b : ack_frame) {
        if (parser.feed(b) == UbxEvent::kAckAck) {
            ++ack_count;
        }
    }
    CHECK(ack_count == 1);
    CHECK(parser.ack_class() == 0x06);
    CHECK(parser.ack_id() == 0x8A);

    auto nak_frame = make_ubx_frame(0x05, 0x00, ack_payload, 2);
    int nak_count = 0;
    for (uint8_t b : nak_frame) {
        if (parser.feed(b) == UbxEvent::kAckNak) {
            ++nak_count;
        }
    }
    CHECK(nak_count == 1);
    CHECK(parser.ack_class() == 0x06);
    CHECK(parser.ack_id() == 0x8A);

    // 5. Length field 200 -> dropped, parser resyncs
    parser.reset();
    std::vector<uint8_t> large_frame = {0xB5, 0x62, 0x01, 0x08, 200, 0};
    for (size_t i = 0; i < 200; ++i) large_frame.push_back(0);
    large_frame.push_back(0); large_frame.push_back(0); // dummy ck
    large_frame.insert(large_frame.end(), pvt_frame.begin(), pvt_frame.end());

    pvt_count = 0;
    for (uint8_t b : large_frame) {
        if (parser.feed(b) == UbxEvent::kNavPvt) {
            ++pvt_count;
        }
    }
    CHECK(pvt_count == 1);

    // 6. ubx_build_valset output
    uint8_t buf[128];
    uint16_t len = ubx_build_default_valset(buf, sizeof(buf));
    CHECK(len > 0);
    CHECK(buf[0] == 0xB5);
    CHECK(buf[1] == 0x62);
    CHECK(buf[2] == 0x06);
    CHECK(buf[3] == 0x8A);
    uint16_t payload_len = static_cast<uint16_t>(buf[4] | (static_cast<uint16_t>(buf[5]) << 8));
    CHECK(len == 6 + payload_len + 2);

    uint8_t expected_a = 0, expected_b = 0;
    compute_fletcher8(buf + 2, 4 + payload_len, expected_a, expected_b);
    CHECK(buf[len - 2] == expected_a);
    CHECK(buf[len - 1] == expected_b);

    if (g_test_failures == 0) {
        std::cout << "All test_ubx_parser tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << g_test_failures << " tests failed in test_ubx_parser!" << std::endl;
        return 1;
    }
}

