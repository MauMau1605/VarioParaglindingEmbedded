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

if __name__ == "__main__":
    test_baro_math()
    test_vario_dt()
    print("All python test validations passed!")
