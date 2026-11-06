#ifndef VARIO_DRIVERS_UBX_PARSER_HPP
#define VARIO_DRIVERS_UBX_PARSER_HPP

#include <cstdint>
#include <cstddef>

namespace vario::drivers {

struct __attribute__((packed)) UbxNavPvt {
    uint32_t iTOW;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  valid;
    uint32_t tAcc;
    int32_t  nano;
    uint8_t  fixType;
    uint8_t  flags;
    uint8_t  flags2;
    uint8_t  numSV;
    int32_t  lon;
    int32_t  lat;
    int32_t  height;
    int32_t  hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t  velN;
    int32_t  velE;
    int32_t  velD;
    int32_t  gSpeed;
    int32_t  headMot;
    uint32_t sAcc;
    uint32_t headAcc;
    uint16_t pDOP;
    uint8_t  flags3;
    uint8_t  reserved1[5];
    int32_t  headVeh;
    int16_t  magDec;
    uint16_t magAcc;
};
static_assert(sizeof(UbxNavPvt) == 92, "UBX-NAV-PVT payload size mismatch");

enum class UbxEvent : uint8_t {
    kNone,
    kNavPvt,
    kAckAck,
    kAckNak
};

struct UbxCfgItem {
    uint32_t key;
    uint32_t value;
    uint8_t  val_size;  // 1, 2, or 4 bytes
};

class UbxParser {
public:
    void reset();
    UbxEvent feed(uint8_t byte);
    const UbxNavPvt& nav_pvt() const { return m_nav_pvt; }
    uint8_t ack_class() const { return m_payload[0]; }
    uint8_t ack_id() const { return m_payload[1]; }

private:
    enum class State : uint8_t {
        kSync1,
        kSync2,
        kClass,
        kId,
        kLenL,
        kLenH,
        kPayload,
        kCkA,
        kCkB
    };

    void update_checksum(uint8_t byte);
    UbxEvent evaluate_packet();

    State     m_state = State::kSync1;
    uint8_t   m_class = 0;
    uint8_t   m_id = 0;
    uint8_t   m_ck_a = 0;
    uint8_t   m_ck_b = 0;
    uint8_t   m_ck_a_rx = 0;
    uint16_t  m_len = 0;
    uint16_t  m_idx = 0;
    uint8_t   m_payload[92] = {};
    UbxNavPvt m_nav_pvt = {};
};

uint16_t ubx_build_valset(uint8_t* out_buf, uint16_t max_len, const UbxCfgItem* items, uint8_t item_count);
uint16_t ubx_build_default_valset(uint8_t* out_buf, uint16_t max_len);
uint16_t ubx_build_baud_valset(uint8_t* out_buf, uint16_t max_len, uint32_t baud);

}  // namespace vario::drivers

#endif  // VARIO_DRIVERS_UBX_PARSER_HPP

