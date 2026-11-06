#!/usr/bin/env python3
"""Host test runner for vario math and UBX parser."""

import math
import sys

def test_baro_math():
    print("Testing baro_math...")
    # 1. LUT vs formula
    H = 44330.0
    lut = [
        61785,  62514,  63254,  64004,  64765,  65536,
        66318,  67112,  67917,  68733,  69561,  70401,
        71253,  72118,  72995,  73884,  74787,  75703,
        76632,  77574,  78531,  79501,  80486,  81486,
        82500,  83529,  84574,  85634,  86711,  87803,
        88911,  90037,  91179,  92339,  93516,  94711,
        95925,  97157,  98407,  99678, 100967, 102277,
       103606, 104957, 106328, 107721
    ]
    for i in range(46):
        h = -500 + i * 100
        ref = round(65536.0 * (1.0 - h / H) ** -5.255)
        assert abs(lut[i] - ref) <= 1, f"LUT[{i}] mismatch: {lut[i]} vs {ref}"

    def clamp_alt_cm(alt_cm):
        return max(-50000, min(400000, alt_cm))

    def scale_from_alt_cm(alt_cm):
        off = clamp_alt_cm(alt_cm) - (-50000)
        idx = off // 10000
        rem = off % 10000
        if idx >= 45:
            idx = 44
            rem = 10000
        s0 = lut[idx]
        s1 = lut[idx + 1]
        return s0 + ((s1 - s0) * rem) // 10000

    def alt_cm_from_scale(scale):
        if scale <= lut[0]: return -50000
        if scale >= lut[-1]: return 400000
        lo, hi = 0, 45
        while lo + 1 < hi:
            mid = (lo + hi) // 2
            if lut[mid] <= scale: lo = mid
            else: hi = mid
        s0, s1 = lut[lo], lut[hi]
        base_cm = -50000 + lo * 10000
        return base_cm + ((scale - s0) * 10000) // (s1 - s0)

    def alt_cm_from_qnh(pressure_cpa, qnh_pa):
        if pressure_cpa <= 0 or qnh_pa <= 0: return 0
        q = qnh_pa * 100
        return alt_cm_from_scale((q << 16) // pressure_cpa)

    def qnh_pa_from_alt(pressure_cpa, alt_mm):
        if pressure_cpa <= 0: return 101325
        scale = scale_from_alt_cm(alt_mm // 10)
        p0_cpa = (pressure_cpa * scale) >> 16
        return (p0_cpa + 50) // 100

    def vario_cm_s_from_dp(dp_cpa_per_s, pressure_pa, alt_cm):
        if pressure_pa <= 0: return 0
        t_dk = 2882 - (65 * clamp_alt_cm(alt_cm)) // 100000
        k = (29271 * t_dk) // 10000
        return (-dp_cpa_per_s * k) // pressure_pa

    # 2. Accuracy vs physics
    for h in range(-500, 4001, 50):
        P = 101325.0 * (1.0 - h / H) ** 5.255
        cpa = round(P * 100)
        calc_h = alt_cm_from_qnh(cpa, 101325) / 100.0
        assert abs(calc_h - h) <= 0.5, f"h={h}, calc={calc_h}"

    # 3. Round trip
    for alt_m in [-300, 0, 500, 1234, 2000, 3000, 3900]:
        P = 101325.0 * (1.0 - (alt_m + 400) / H) ** 5.255
        cpa = round(P * 100)
        qnh = qnh_pa_from_alt(cpa, alt_m * 1000)
        calc_alt_m = alt_cm_from_qnh(cpa, qnh) / 100.0
        assert abs(calc_alt_m - alt_m) <= 0.25, f"alt={alt_m}, calc={calc_alt_m}"

    # 5. Vario scale
    vz_sl = vario_cm_s_from_dp(-1200, 101325, 0)
    assert abs(vz_sl - 99) <= 1, f"vz_sl = {vz_sl}"

    t_ref = 288.15 - 0.0065 * 2500.0
    vz_ref = 12.0 * (29.271 * t_ref) / 74675.0 * 100.0
    vz_alt = vario_cm_s_from_dp(-1200, 74675, 250000)
    assert abs(vz_alt - vz_ref) <= 1.5, f"vz_alt = {vz_alt}, ref = {vz_ref}"

    print("baro_math tests passed!")

def test_vario_dt():
    print("Testing vario_dt...")
    def quantize_baro_dt_ms(elapsed_ms):
        n = (elapsed_ms + 10) // 20
        if n < 1: n = 1
        return n * 20

    assert quantize_baro_dt_ms(19) == 20
    assert quantize_baro_dt_ms(20) == 20
    assert quantize_baro_dt_ms(21) == 20
    assert quantize_baro_dt_ms(38) == 40
    assert quantize_baro_dt_ms(41) == 40
    assert quantize_baro_dt_ms(0) == 20
    assert quantize_baro_dt_ms(5) == 20
    assert quantize_baro_dt_ms(9) == 20
    print("vario_dt tests passed!")

def test_ubx_parser():
    print("Testing ubx_parser...")
    def fletcher8(data):
        a, b = 0, 0
        for byte in data:
            a = (a + byte) & 0xFF
            b = (b + a) & 0xFF
        return a, b

    def make_ubx(cls, msg_id, payload):
        length = len(payload)
        header = bytes([0xB5, 0x62, cls, msg_id, length & 0xFF, (length >> 8) & 0xFF])
        ck_a, ck_b = fletcher8(header[2:] + payload)
        return header + payload + bytes([ck_a, ck_b])

    class PyUbxParser:
        def __init__(self):
            self.reset()

        def reset(self):
            self.state = 0
            self.cls = 0
            self.id = 0
            self.len = 0
            self.payload = bytearray()
            self.ck_a = 0
            self.ck_b = 0

        def feed(self, b):
            if self.state == 0:
                if b == 0xB5: self.state = 1
            elif self.state == 1:
                if b == 0x62: self.state = 2
                elif b != 0xB5: self.state = 0
            elif self.state == 2:
                self.cls = b
                self.ck_a = b
                self.ck_b = b
                self.state = 3
            elif self.state == 3:
                self.id = b
                self.ck_a = (self.ck_a + b) & 0xFF
                self.ck_b = (self.ck_b + self.ck_a) & 0xFF
                self.state = 4
            elif self.state == 4:
                self.len = b
                self.ck_a = (self.ck_a + b) & 0xFF
                self.ck_b = (self.ck_b + self.ck_a) & 0xFF
                self.state = 5
            elif self.state == 5:
                self.len |= (b << 8)
                self.ck_a = (self.ck_a + b) & 0xFF
                self.ck_b = (self.ck_b + self.ck_a) & 0xFF
                if self.len > 92:
                    self.state = 0
                else:
                    self.payload = bytearray()
                    self.state = 6 if self.len > 0 else 7
            elif self.state == 6:
                self.payload.append(b)
                self.ck_a = (self.ck_a + b) & 0xFF
                self.ck_b = (self.ck_b + self.ck_a) & 0xFF
                if len(self.payload) == self.len:
                    self.state = 7
            elif self.state == 7:
                if b == self.ck_a:
                    self.state = 8
                else:
                    self.state = 0
            elif self.state == 8:
                self.state = 0
                if b == self.ck_b:
                    if self.cls == 0x01 and self.id == 0x07 and self.len == 92:
                        return "NAV_PVT"
                    elif self.cls == 0x05 and self.id == 0x01 and self.len == 2:
                        return "ACK_ACK"
                    elif self.cls == 0x05 and self.id == 0x00 and self.len == 2:
                        return "ACK_NAK"
            return None

    def collect_events(parser, data):
        evs = []
        for b in data:
            ev = parser.feed(b)
            if ev is not None:
                evs.append(ev)
        return evs

    # Test 1: NAV-PVT
    parser = PyUbxParser()
    pvt_payload = bytes(92)
    pvt_frame = make_ubx(0x01, 0x07, pvt_payload)
    events = collect_events(parser, pvt_frame)
    assert events == ["NAV_PVT"], f"Expected NAV_PVT, got {events}"

    # Test 2: NMEA stream prepended
    parser.reset()
    nmea = b"$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
    stream = nmea + pvt_frame
    events = collect_events(parser, stream)
    assert events == ["NAV_PVT"]

    # Test 3: Corrupted payload byte + recovery
    parser.reset()
    corrupt = bytearray(pvt_frame)
    corrupt[15] ^= 0xFF
    events = collect_events(parser, corrupt + pvt_frame)
    assert events == ["NAV_PVT"], f"Expected 1 NAV_PVT on recovery, got {events}"

    # Test 4: ACK-ACK and ACK-NAK
    parser.reset()
    ack_frame = make_ubx(0x05, 0x01, bytes([0x06, 0x8A]))
    events = collect_events(parser, ack_frame)
    assert events == ["ACK_ACK"], f"Expected ACK_ACK, got {events}"

    parser.reset()
    nak_frame = make_ubx(0x05, 0x00, bytes([0x06, 0x8A]))
    events = collect_events(parser, nak_frame)
    assert events == ["ACK_NAK"], f"Expected ACK_NAK, got {events}"

    # Test 5: Length > 92 dropped, then next frame passes
    parser.reset()
    big_frame = make_ubx(0x01, 0x07, bytes(150))
    events = collect_events(parser, big_frame + pvt_frame)
    assert events == ["NAV_PVT"], f"Expected NAV_PVT, got {events}"

    print("ubx_parser tests passed!")

if __name__ == "__main__":
    test_baro_math()
    test_vario_dt()
    test_ubx_parser()
    print("All python test validations passed!")
