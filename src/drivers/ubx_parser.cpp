#include "drivers/ubx_parser.hpp"
#include <string.h>

namespace vario::drivers {

void UbxParser::reset() {
    m_state = State::kSync1;
    m_class = 0;
    m_id = 0;
    m_ck_a = 0;
    m_ck_b = 0;
    m_len = 0;
    m_idx = 0;
}

void UbxParser::update_checksum(uint8_t byte) {
    m_ck_a = static_cast<uint8_t>(m_ck_a + byte);
    m_ck_b = static_cast<uint8_t>(m_ck_b + m_ck_a);
}

UbxEvent UbxParser::evaluate_packet() {
    if (m_class == 0x01 && m_id == 0x07 && m_len == sizeof(UbxNavPvt)) {
        memcpy(&m_nav_pvt, m_payload, sizeof(UbxNavPvt));
        return UbxEvent::kNavPvt;
    }
    if (m_class == 0x05 && m_id == 0x01 && m_len >= 2) {
        return UbxEvent::kAckAck;
    }
    if (m_class == 0x05 && m_id == 0x00 && m_len >= 2) {
        return UbxEvent::kAckNak;
    }
    return UbxEvent::kNone;
}

UbxEvent UbxParser::feed(uint8_t byte) {
    switch (m_state) {
    case State::kSync1:
        if (byte == 0xB5) { m_state = State::kSync2; }
        return UbxEvent::kNone;
    case State::kSync2:
        if (byte == 0x62) {
            m_ck_a = 0; m_ck_b = 0;
            m_state = State::kClass;
        } else if (byte != 0xB5) {
            m_state = State::kSync1;
        }
        return UbxEvent::kNone;
    case State::kClass:
        m_class = byte; update_checksum(byte);
        m_state = State::kId;
        return UbxEvent::kNone;
    case State::kId:
        m_id = byte; update_checksum(byte);
        m_state = State::kLenL;
        return UbxEvent::kNone;
    case State::kLenL:
        m_len = byte; update_checksum(byte);
        m_state = State::kLenH;
        return UbxEvent::kNone;
    case State::kLenH:
        m_len = static_cast<uint16_t>(m_len | (static_cast<uint16_t>(byte) << 8));
        update_checksum(byte);
        m_idx = 0;
        if (m_len > sizeof(m_payload)) {
            m_state = (byte == 0xB5) ? State::kSync2 : State::kSync1;
            return UbxEvent::kNone;
        }
        m_state = (m_len == 0) ? State::kCkA : State::kPayload;
        return UbxEvent::kNone;
    case State::kPayload:
        m_payload[m_idx++] = byte; update_checksum(byte);
        if (m_idx >= m_len) { m_state = State::kCkA; }
        return UbxEvent::kNone;
    case State::kCkA:
        m_ck_a_rx = byte;
        m_state = State::kCkB;
        return UbxEvent::kNone;
    case State::kCkB:
        m_state = (byte == 0xB5) ? State::kSync2 : State::kSync1;
        if (m_ck_a_rx == m_ck_a && byte == m_ck_b) {
            return evaluate_packet();
        }
        return UbxEvent::kNone;
    }
    return UbxEvent::kNone;
}

static uint16_t append_u4_le(uint8_t* buf, uint32_t val) {
    buf[0] = static_cast<uint8_t>(val & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    return 4;
}

uint16_t ubx_build_valset(uint8_t* out_buf, uint16_t max_len, const UbxCfgItem* items, uint8_t item_count) {
    uint16_t payload_len = 4;
    for (uint8_t i = 0; i < item_count; ++i) {
        payload_len = static_cast<uint16_t>(payload_len + 4 + items[i].val_size);
    }
    if (max_len < 6 + payload_len + 2) { return 0; }

    out_buf[0] = 0xB5; out_buf[1] = 0x62; // Sync
    out_buf[2] = 0x06; out_buf[3] = 0x8A; // Class, ID
    out_buf[4] = static_cast<uint8_t>(payload_len & 0xFF);
    out_buf[5] = static_cast<uint8_t>((payload_len >> 8) & 0xFF);
    out_buf[6] = 0x00; out_buf[7] = 0x01; // Version 0, Layer 1 (RAM)
    out_buf[8] = 0x00; out_buf[9] = 0x00; // Reserved

    uint16_t pos = 10;
    for (uint8_t i = 0; i < item_count; ++i) {
        pos = static_cast<uint16_t>(pos + append_u4_le(&out_buf[pos], items[i].key));
        for (uint8_t b = 0; b < items[i].val_size; ++b) {
            out_buf[pos++] = static_cast<uint8_t>((items[i].value >> (8 * b)) & 0xFF);
        }
    }

    uint8_t ck_a = 0, ck_b = 0;
    for (uint16_t i = 2; i < 6 + payload_len; ++i) {
        ck_a = static_cast<uint8_t>(ck_a + out_buf[i]);
        ck_b = static_cast<uint8_t>(ck_b + ck_a);
    }
    out_buf[pos++] = ck_a;
    out_buf[pos++] = ck_b;
    return pos;
}

uint16_t ubx_build_default_valset(uint8_t* out_buf, uint16_t max_len) {
    static constexpr UbxCfgItem kDefaultCfg[] = {
        {0x10740002, 0, 1},       // CFG-UART1OUTPROT-NMEA = 0
        {0x10740001, 1, 1},       // CFG-UART1OUTPROT-UBX = 1
        {0x20910007, 1, 1},       // CFG-MSGOUT-UBX_NAV_PVT_UART1 = 1
        {0x30210001, 1000, 2},    // CFG-RATE-MEAS = 1000 ms
        {0x20110021, 6, 1},       // CFG-NAVSPG-DYNMODEL = 6 (Airborne < 1g)
        {0x1031001F, 1, 1},       // CFG-SIGNAL-GPS_ENA = 1
        {0x10310021, 1, 1},       // CFG-SIGNAL-GAL_ENA = 1
        {0x10310022, 0, 1},       // CFG-SIGNAL-BDS_ENA = 0
        {0x10310025, 0, 1},       // CFG-SIGNAL-GLO_ENA = 0
        {0x20D00001, 2, 1}        // CFG-PM-OPERATEMODE = 2 (PSMCT)
    };
    return ubx_build_valset(out_buf, max_len, kDefaultCfg, sizeof(kDefaultCfg) / sizeof(kDefaultCfg[0]));
}

uint16_t ubx_build_baud_valset(uint8_t* out_buf, uint16_t max_len, uint32_t baud) {
    const UbxCfgItem baud_cfg = {0x40520001, baud, 4}; // CFG-UART1-BAUDRATE
    return ubx_build_valset(out_buf, max_len, &baud_cfg, 1);
}

}  // namespace vario::drivers

