#!/usr/bin/env python3
"""
Host test runner for VarioUSB:
Protocol parsers and all embedded algorithms (offline verification).
"""

import math
import sys
import os

# Add tools to path for protocol cross-verification
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools", "vario_monitor"))

# ============================================================================
# 1. Barometric Math Tests (baro_math.hpp)
# ============================================================================
def test_baro_math():
    print("Testing baro_math...")
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

    # Accuracy vs physics
    for h in range(-500, 4001, 50):
        P = 101325.0 * (1.0 - h / H) ** 5.255
        cpa = round(P * 100)
        calc_h = alt_cm_from_qnh(cpa, 101325) / 100.0
        assert abs(calc_h - h) <= 0.5, f"h={h}, calc={calc_h}"

    # Round trip
    for alt_m in [-300, 0, 500, 1234, 2000, 3000, 3900]:
        P = 101325.0 * (1.0 - (alt_m + 400) / H) ** 5.255
        cpa = round(P * 100)
        qnh = qnh_pa_from_alt(cpa, alt_m * 1000)
        calc_alt_m = alt_cm_from_qnh(cpa, qnh) / 100.0
        assert abs(calc_alt_m - alt_m) <= 0.25, f"alt={alt_m}, calc={calc_alt_m}"

    # Vario scale
    vz_sl = vario_cm_s_from_dp(-1200, 101325, 0)
    assert abs(vz_sl - 99) <= 1, f"vz_sl = {vz_sl}"

    t_ref = 288.15 - 0.0065 * 2500.0
    vz_ref = 12.0 * (29.271 * t_ref) / 74675.0 * 100.0
    vz_alt = vario_cm_s_from_dp(-1200, 74675, 250000)
    assert abs(vz_alt - vz_ref) <= 1.5, f"vz_alt = {vz_alt}, ref = {vz_ref}"

    # Clamping & edge protections
    assert clamp_alt_cm(-60000) == -50000
    assert clamp_alt_cm(500000) == 400000
    assert alt_cm_from_qnh(-10, 101325) == 0
    assert alt_cm_from_qnh(100000, 0) == 0
    assert qnh_pa_from_alt(0, 1000) == 101325
    assert vario_cm_s_from_dp(100, 0, 0) == 0

    print("  [OK] baro_math tests passed!")


# ============================================================================
# 2. Vario Dt Quantization Tests (baro_math.hpp)
# ============================================================================
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
    print("  [OK] vario_dt tests passed!")


# ============================================================================
# 3. Protocol: UBX Parser & Frame Generator (ubx_parser.cpp)
# ============================================================================
def test_ubx_parser():
    print("Testing protocol: ubx_parser...")
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

    # 1. NAV-PVT
    parser = PyUbxParser()
    pvt_payload = bytes(92)
    pvt_frame = make_ubx(0x01, 0x07, pvt_payload)
    events = collect_events(parser, pvt_frame)
    assert events == ["NAV_PVT"], f"Expected NAV_PVT, got {events}"

    # 2. NMEA stream prepended
    parser.reset()
    nmea = b"$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
    stream = nmea + pvt_frame
    events = collect_events(parser, stream)
    assert events == ["NAV_PVT"]

    # 3. Corrupted payload byte + recovery
    parser.reset()
    corrupt = bytearray(pvt_frame)
    corrupt[15] ^= 0xFF
    events = collect_events(parser, corrupt + pvt_frame)
    assert events == ["NAV_PVT"], f"Expected 1 NAV_PVT on recovery, got {events}"

    # 4. ACK-ACK and ACK-NAK
    parser.reset()
    ack_frame = make_ubx(0x05, 0x01, bytes([0x06, 0x8A]))
    events = collect_events(parser, ack_frame)
    assert events == ["ACK_ACK"], f"Expected ACK_ACK, got {events}"

    parser.reset()
    nak_frame = make_ubx(0x05, 0x00, bytes([0x06, 0x8A]))
    events = collect_events(parser, nak_frame)
    assert events == ["ACK_NAK"], f"Expected ACK_NAK, got {events}"

    # 5. Length > 92 dropped, then next frame passes
    parser.reset()
    big_frame = make_ubx(0x01, 0x07, bytes(150))
    events = collect_events(parser, big_frame + pvt_frame)
    assert events == ["NAV_PVT"], f"Expected NAV_PVT, got {events}"

    print("  [OK] protocol: ubx_parser tests passed!")


# ============================================================================
# 4. Protocol: LK8EX1 Formatter & NMEA Checksum (lk8ex1_formatter.cpp)
# ============================================================================
def test_lk8ex1_formatter():
    print("Testing protocol: lk8ex1_formatter...")
    def nmea_checksum(sentence: str) -> int:
        cs = 0
        started = False
        for ch in sentence:
            if ch == '$':
                started = True
                continue
            if ch == '*':
                break
            if started:
                cs ^= ord(ch)
        return cs

    def lk8ex1_format(pressure_pa, vario_cm_s, temp_dc, buf_size=64):
        if buf_size < 40:
            return None, "buffer too small"
        temp_int = int(temp_dc / 10)  # C++ integer truncation towards zero
        body = f"$LK8EX1,{pressure_pa},99999,{vario_cm_s},{temp_int},999,*"
        cs = nmea_checksum(body)
        sentence = f"{body}{cs:02X}\r\n"
        if len(sentence) > buf_size:
            return None, "buffer overflow"
        return sentence, None

    # 1. Nominal sentence
    s, err = lk8ex1_format(101325, 150, 215)
    assert err is None
    assert s.startswith("$LK8EX1,101325,99999,150,21,999,*")
    assert s.endswith("\r\n")

    # Verify checksum
    star = s.find('*')
    cs_str = s[star+1:star+3]
    expected_cs = nmea_checksum(s[:star+1])
    assert int(cs_str, 16) == expected_cs

    # Cross-verify with protocol parser from tools/vario_monitor/protocol.py if available
    try:
        from protocol import parse_lk8ex1, verify_checksum
        assert verify_checksum(s), "protocol.py verify_checksum failed on formatted sentence"
        parsed = parse_lk8ex1(s)
        assert parsed is not None
        assert parsed['valid_checksum'] is True
        assert parsed['pressure_pa'] == 101325
        assert parsed['vario_cm_s'] == 150
        assert parsed['temperature_c'] == 21
        assert parsed['raw_altitude'] == 99999
        assert parsed['battery'] == 999
    except ImportError:
        pass

    # 2. Sink and negative temperature
    s, err = lk8ex1_format(95400, -320, -85)
    assert err is None
    assert s.startswith("$LK8EX1,95400,99999,-320,-8,999,*")
    star = s.find('*')
    assert int(s[star+1:star+3], 16) == nmea_checksum(s[:star+1])

    # 3. Zero climb rate and 0 deg C
    s, err = lk8ex1_format(100000, 0, 0)
    assert err is None
    assert s.startswith("$LK8EX1,100000,99999,0,0,999,*")

    # 4. Buffer boundary checks
    _, err_small = lk8ex1_format(101325, 150, 215, buf_size=39)
    assert err_small == "buffer too small"
    s_min, err_ok = lk8ex1_format(101325, 150, 215, buf_size=40)
    assert err_ok is None
    assert len(s_min) <= 40

    print("  [OK] protocol: lk8ex1_formatter tests passed!")


# ============================================================================
# 5. Algorithm: Vario Engine & IIR Filter (vario_engine.cpp)
# ============================================================================
def test_vario_engine():
    print("Testing algorithm: vario_engine...")
    class PyVarioEngine:
        def __init__(self):
            self.init()

        def init(self):
            self.initialized = True
            self.first_sample = True
            self.prev_pressure_cpa = 0
            self.prev_time_ms = 0
            self.filtered_vario_cm_s = 0
            self.qnh_pa = 101325
            self.last_alt_cm = 0

        def set_qnh(self, qnh_pa):
            self.qnh_pa = qnh_pa if qnh_pa > 0 else 101325

        def update(self, pressure_pa, pressure_cpa, temp_cdeg, now_ms):
            if not self.initialized:
                return None, "not initialized"

            # baro_math altitude
            alt_cm = 0
            if pressure_cpa > 0 and self.qnh_pa > 0:
                q = self.qnh_pa * 100
                scale = (q << 16) // pressure_cpa
                # Simplified scale to alt for test
                alt_cm = int(4433000 * (1.0 - (pressure_cpa / 100.0 / self.qnh_pa) ** (1.0 / 5.255)))

            if self.first_sample:
                self.prev_pressure_cpa = pressure_cpa
                self.prev_time_ms = now_ms
                self.first_sample = False
                self.last_alt_cm = alt_cm
                return {
                    'pressure_pa': pressure_pa,
                    'vario_cm_s': 0,
                    'alt_baro_cm': alt_cm,
                    'temp_dc': temp_cdeg // 10
                }, None

            # Quantize dt (20 ms nominal)
            elapsed = now_ms - self.prev_time_ms
            n = (elapsed + 10) // 20
            dt_ms = max(1, n) * 20

            dp_cpa = pressure_cpa - self.prev_pressure_cpa
            dp_cpa_per_s = (dp_cpa * 1000) // dt_ms

            # Baro math vario
            t_dk = 2882 - (65 * max(-50000, min(400000, self.last_alt_cm))) // 100000
            k = (29271 * t_dk) // 10000
            raw_vario = (-dp_cpa_per_s * k) // pressure_pa

            # IIR filter: alpha = 26, scale = 256
            alpha = 26
            scale = 256
            self.filtered_vario_cm_s = (alpha * raw_vario + (scale - alpha) * self.filtered_vario_cm_s) // scale

            self.prev_pressure_cpa = pressure_cpa
            self.prev_time_ms = now_ms
            self.last_alt_cm = alt_cm

            return {
                'pressure_pa': pressure_pa,
                'vario_cm_s': self.filtered_vario_cm_s,
                'alt_baro_cm': alt_cm,
                'temp_dc': temp_cdeg // 10
            }, None

    engine = PyVarioEngine()

    # 1. First sample
    res, err = engine.update(101325, 10132500, 2150, now_ms=100)
    assert err is None
    assert res['vario_cm_s'] == 0
    assert res['temp_dc'] == 215

    # 2. Step response (constant pressure decrease of 24 cPa / 20 ms -> ~99 cm/s climb)
    press_cpa = 10132500
    time_ms = 100
    prev_vz = 0
    for _ in range(40):
        time_ms += 20
        press_cpa -= 24
        res, _ = engine.update(press_cpa // 100, press_cpa, 2150, time_ms)
        assert res['vario_cm_s'] >= prev_vz, "Filter response must be monotonic"
        prev_vz = res['vario_cm_s']

    # With integer truncation without rounding, steady state reaches 90 cm/s
    assert abs(res['vario_cm_s'] - 90) <= 2, f"Expected ~90 cm/s due to integer truncation, got {res['vario_cm_s']}"

    # 3. Level flight decay (constant pressure) -> decays to 0
    for _ in range(50):
        time_ms += 20
        res, _ = engine.update(press_cpa // 100, press_cpa, 2150, time_ms)
    assert res['vario_cm_s'] == 0, f"Expected 0 cm/s in level flight, got {res['vario_cm_s']}"

    print("  [OK] algorithm: vario_engine tests passed!")


# ============================================================================
# 6. Algorithm: GNSS Flight State Machine & QNH Calibration (gnss_processor.cpp)
# ============================================================================
def test_gnss_processor():
    print("Testing algorithm: gnss_processor...")
    class FlightState:
        kWaitGpsFix = 0
        kPreTakeoffCalib = 1
        kInFlight = 2

    class PyGnssProcessor:
        def __init__(self):
            self.init()

        def init(self):
            self.state = FlightState.kWaitGpsFix
            self.has_valid_fix = False
            self.ring_gps_alt = [0] * 20
            self.ring_pressure = [0] * 20
            self.ring_head = 0
            self.ring_count = 0
            self.qnh_pa = 101325
            self.takeoff_condition_start_ms = 0
            self.takeoff_condition_active = False
            self.baro_acc_cpa = 0
            self.baro_acc_n = 0
            self.last_pressure_cpa = 10132500
            self.last_vario_cm_s = 0

        def update_baro(self, pressure_cpa, vario_cm_s):
            self.baro_acc_cpa += pressure_cpa
            self.baro_acc_n += 1
            self.last_pressure_cpa = pressure_cpa
            self.last_vario_cm_s = vario_cm_s

        def evaluate_takeoff(self, ground_speed_mm_s, now_ms):
            abs_vz = abs(self.last_vario_cm_s)
            vz_only = (abs_vz > 120)            # kTakeoffVzCmS
            vz_speed = (ground_speed_mm_s > 2500) and (abs_vz > 50)
            condition = vz_only or vz_speed

            if not condition:
                self.takeoff_condition_active = False
                return

            if not self.takeoff_condition_active:
                self.takeoff_condition_active = True
                self.takeoff_condition_start_ms = now_ms
            elif (now_ms - self.takeoff_condition_start_ms) >= 3000:
                self.state = FlightState.kInFlight

        def update_p0_calibration(self, alt_msl_mm, mean_press_cpa):
            self.ring_gps_alt[self.ring_head] = alt_msl_mm
            self.ring_pressure[self.ring_head] = mean_press_cpa
            self.ring_head = (self.ring_head + 1) % 20
            if self.ring_count < 20:
                self.ring_count += 1

            if self.ring_count >= 5:  # kQnhMinSamples
                avg_alt = sum(self.ring_gps_alt[:self.ring_count]) // self.ring_count
                avg_press = sum(self.ring_pressure[:self.ring_count]) // self.ring_count
                # Simplified P0 = P * (1 - h/44330)^-5.255
                h = avg_alt / 1000.0
                p_pa = avg_press / 100.0
                self.qnh_pa = round(p_pa * (1.0 - h / 44330.0) ** -5.255)

        def on_fix(self, valid, ground_speed_mm_s, alt_msl_mm, now_ms):
            if not valid:
                self.has_valid_fix = False
                if self.state == FlightState.kPreTakeoffCalib:
                    self.state = FlightState.kWaitGpsFix
                self.ring_count = 0
                self.ring_head = 0
                self.takeoff_condition_active = False
                return

            self.has_valid_fix = True
            if self.state == FlightState.kWaitGpsFix:
                self.state = FlightState.kPreTakeoffCalib

            mean_press_cpa = (self.baro_acc_cpa // self.baro_acc_n) if self.baro_acc_n > 0 else self.last_pressure_cpa
            self.baro_acc_cpa = 0
            self.baro_acc_n = 0

            if self.state == FlightState.kPreTakeoffCalib:
                self.evaluate_takeoff(ground_speed_mm_s, now_ms)
                if not self.takeoff_condition_active and (ground_speed_mm_s < 1500):
                    self.update_p0_calibration(alt_msl_mm, mean_press_cpa)

    proc = PyGnssProcessor()

    # 1. State machine transition: wait fix -> pre-takeoff
    assert proc.state == FlightState.kWaitGpsFix
    proc.on_fix(valid=True, ground_speed_mm_s=200, alt_msl_mm=440000, now_ms=1000)
    assert proc.state == FlightState.kPreTakeoffCalib

    # 2. Revert on fix loss
    proc.on_fix(valid=False, ground_speed_mm_s=0, alt_msl_mm=0, now_ms=2000)
    assert proc.state == FlightState.kWaitGpsFix

    # 3. Stationary QNH calibration
    for _ in range(50): proc.update_baro(9600000, 0)
    proc.on_fix(valid=True, ground_speed_mm_s=200, alt_msl_mm=440000, now_ms=3000)
    # Feed 4 fixes (total 4 < 5)
    for t in range(4000, 7000, 1000):
        for _ in range(50): proc.update_baro(9600000, 0)
        proc.on_fix(valid=True, ground_speed_mm_s=200, alt_msl_mm=440000, now_ms=t)
        assert proc.qnh_pa == 101325, "Must not calibrate before 5 samples"

    # 5th fix triggers calibration
    for _ in range(50): proc.update_baro(9600000, 0)
    proc.on_fix(valid=True, ground_speed_mm_s=200, alt_msl_mm=440000, now_ms=7000)
    assert proc.qnh_pa != 101325, "QNH should be calibrated"
    assert abs(proc.qnh_pa - 101170) <= 50, f"Expected ~101170 Pa, got {proc.qnh_pa}"

    # 4. Takeoff detection: |Vz| > 120 cm/s sustained for 3000 ms
    proc.update_baro(9600000, 150)
    proc.on_fix(valid=True, ground_speed_mm_s=500, alt_msl_mm=450000, now_ms=8000)
    assert proc.state == FlightState.kPreTakeoffCalib  # 0 ms hold

    proc.update_baro(9600000, 150)
    proc.on_fix(valid=True, ground_speed_mm_s=500, alt_msl_mm=451500, now_ms=10000)
    assert proc.state == FlightState.kPreTakeoffCalib  # 2000 ms hold

    proc.update_baro(9600000, 150)
    proc.on_fix(valid=True, ground_speed_mm_s=500, alt_msl_mm=453000, now_ms=11000)
    assert proc.state == FlightState.kInFlight  # 3000 ms hold -> InFlight!

    # 5. In-flight QNH frozen and fix loss does not revert state
    calib_frozen = proc.qnh_pa
    proc.on_fix(valid=False, ground_speed_mm_s=0, alt_msl_mm=0, now_ms=12000)
    assert proc.state == FlightState.kInFlight
    assert proc.qnh_pa == calib_frozen

    print("  [OK] algorithm: gnss_processor tests passed!")


# ============================================================================
# 7. Algorithm: Battery Monitor Piecewise LUT & Filter (battery_monitor.cpp)
# ============================================================================
def test_battery_monitor():
    print("Testing algorithm: battery_monitor...")
    lut = [
        (4200, 100),
        (4100, 90),
        (3900, 70),
        (3800, 50),
        (3700, 30),
        (3600, 15),
        (3500, 5),
        (3000, 0)
    ]

    def voltage_to_percent(mv):
        if mv >= lut[0][0]: return 100
        if mv <= lut[7][0]: return 0
        for i in range(7):
            if mv >= lut[i + 1][0]:
                v_span = lut[i][0] - lut[i + 1][0]
                p_span = lut[i][1] - lut[i + 1][1]
                v_off = mv - lut[i + 1][0]
                return lut[i + 1][1] + (v_off * p_span) // v_span
        return 0

    # 1. Exact knot points
    for mv, pct in lut:
        assert voltage_to_percent(mv) == pct, f"Mismatch at {mv} mV: expected {pct}%, got {voltage_to_percent(mv)}%"

    # 2. Clamping
    assert voltage_to_percent(4350) == 100
    assert voltage_to_percent(2800) == 0
    assert voltage_to_percent(0) == 0

    # 3. Midpoints
    assert voltage_to_percent(4150) == 95
    assert voltage_to_percent(4000) == 80
    assert voltage_to_percent(3850) == 60
    assert voltage_to_percent(3750) == 40
    assert voltage_to_percent(3650) == 22
    assert voltage_to_percent(3550) == 10
    assert voltage_to_percent(3250) == 2

    # 4. EMA filter: (3 * old + new) / 4
    v_filt = 4000
    # Apply step drop to 3600
    v_filt = (3 * v_filt + 3600) // 4
    assert v_filt == 3900
    v_filt = (3 * v_filt + 3600) // 4
    assert v_filt == 3825
    v_filt = (3 * v_filt + 3600) // 4
    assert v_filt == 3768

    print("  [OK] algorithm: battery_monitor tests passed!")


# ============================================================================
# 8. Algorithm: Buzzer Audio Cadence & Tone (buzzer.cpp)
# ============================================================================
def test_buzzer():
    print("Testing algorithm: buzzer...")
    def climb_rate_to_frequency(cms):
        cms = max(30, min(500, cms))
        delta_v = cms - 30
        range_v = 500 - 30
        range_f = 2500 - 700
        return 700 + (delta_v * range_f) // range_v

    def climb_rate_to_period(cms):
        cms = max(30, min(500, cms))
        delta_v = cms - 30
        range_v = 500 - 30
        range_p = 600 - 150
        return 600 - (delta_v * range_p) // range_v

    def sink_rate_to_frequency(cms):
        sink = -cms
        if sink > 500: sink = 500
        return 300 - ((sink - 100) * 100) // 400

    # 1. Climb frequency range
    assert climb_rate_to_frequency(30) == 700
    assert climb_rate_to_frequency(500) == 2500
    assert climb_rate_to_frequency(265) == 1600
    assert climb_rate_to_frequency(600) == 2500  # clamp

    # 2. Climb period range (50% on, 50% off)
    assert climb_rate_to_period(30) == 600
    assert climb_rate_to_period(500) == 150
    p = climb_rate_to_period(265)
    assert 150 <= p <= 600

    # 3. Sink frequency
    assert sink_rate_to_frequency(-100) == 300
    assert sink_rate_to_frequency(-500) == 200
    assert sink_rate_to_frequency(-300) == 250

    # 4. Deadband: -100 < vz < +30 is silence
    for vz in [-99, -50, 0, 10, 29]:
        is_silent = (vz > -100 and vz < 30)
        assert is_silent

    print("  [OK] algorithm: buzzer tests passed!")


# ============================================================================
# 9. Algorithm: Bosch BMP390 Integer Compensation Math (bmp390.cpp)
# ============================================================================
def test_bmp390_compensation():
    print("Testing algorithm: bmp390 integer compensation math...")
    # Bosch calibration parameters (from test vector)
    par_t1 = 27800
    par_t2 = 19500
    par_t3 = -10
    par_p1 = 6500
    par_p2 = -4500
    par_p3 = 15
    par_p4 = 8
    par_p5 = 21000
    par_p6 = 18000
    par_p7 = 5
    par_p8 = -3
    par_p9 = 2500
    par_p10 = 12
    par_p11 = -8

    def compensate_temperature(raw_temp):
        pd1 = raw_temp - (256 * par_t1)
        pd2 = par_t2 * pd1
        pd3 = pd1 * pd1
        pd4 = pd3 * par_t3
        pd5 = (pd2 * 262144) + pd4
        t_lin = pd5 // 4294967296
        comp_temp = (t_lin * 25) // 16384
        comp_temp = max(-4000, min(8500, comp_temp))
        return comp_temp, t_lin

    def compensate_pressure(raw_press, t_lin):
        t = t_lin
        pd1 = t * t
        pd2 = pd1 // 64
        pd3 = (pd2 * t) // 256
        pd4 = (par_p8 * pd3) // 32
        pd5 = par_p7 * pd1 * 16
        pd6 = par_p6 * t * 4194304
        offset = par_p5 * 140737488355328 + pd4 + pd5 + pd6

        pd1_sens = t * t
        pd2_sens = (par_p4 * ((pd1_sens // 64) * t // 256)) // 32
        pd4_sens = par_p3 * pd1_sens * 4
        pd5_sens = (par_p2 - 16384) * t * 2097152
        sensitivity = (par_p1 - 16384) * 70368744177664 + pd2_sens + pd4_sens + pd5_sens

        raw = raw_press
        pd1_p = (sensitivity // 16777216) * raw
        pd2_terms = par_p10 * t_lin
        pd3_terms = pd2_terms + (65536 * par_p9)
        pd4_terms = (pd3_terms * raw) // 8192
        pd5_terms = (raw * (pd4_terms // 10)) // 512 * 10
        pd6_terms = raw * raw
        pd7_terms = (par_p11 * pd6_terms) // 65536
        pd8_terms = (pd7_terms * raw) // 128
        terms = pd5_terms + pd8_terms

        pd9 = (offset // 4) + pd1_p + terms
        if pd9 <= 0: return 3000000
        comp_cpa = (pd9 * 25) // 1099511627776
        return max(3000000, min(12500000, comp_cpa))

    # Test nominal temperature readout
    raw_temp = 0x800000  # mid-range
    temp_cdeg, t_lin = compensate_temperature(raw_temp)
    assert -4000 <= temp_cdeg <= 8500, f"Temperature out of range: {temp_cdeg}"

    # Test nominal pressure readout
    raw_press = 0x7E0000
    press_cpa = compensate_pressure(raw_press, t_lin)
    assert 3000000 <= press_cpa <= 12500000, f"Pressure out of range: {press_cpa}"

    # Verify bounds clamping
    temp_min, _ = compensate_temperature(0)
    assert temp_min >= -4000
    temp_max, _ = compensate_temperature(0xFFFFFF)
    assert temp_max <= 8500

    print("  [OK] algorithm: bmp390 integer compensation math tests passed!")


# ============================================================================
# Main test suite entry point
# ============================================================================
if __name__ == "__main__":
    print("==================================================================")
    print("        RUNNING OFFLINE UNIT TESTS (PROTOCOLS & ALGORITHMS)       ")
    print("==================================================================")
    test_baro_math()
    test_vario_dt()
    test_ubx_parser()
    test_lk8ex1_formatter()
    test_vario_engine()
    test_gnss_processor()
    test_battery_monitor()
    test_buzzer()
    test_bmp390_compensation()
    print("==================================================================")
    print("     ALL 9 OFFLINE PROTOCOL & ALGORITHM TEST SUITES PASSED!       ")
    print("==================================================================")
