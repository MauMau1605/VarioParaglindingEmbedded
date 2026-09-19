"""
Protocol utilities for LK8EX1 NMEA sentences.

Format:
$LK8EX1,pressure,altitude,vario,temperature,battery,*checksum\\r\\n
Example:
$LK8EX1,101325,99999,15,21,999,*3A\\r\\n
"""

import math
from typing import Optional, Tuple, Dict, Any


def compute_checksum(sentence: str) -> str:
    """Compute the 2-hex-digit NMEA XOR checksum for content between '$' and '*'."""
    cs = 0
    # Strip $ and * and anything after
    start = sentence.find('$')
    if start != -1:
        sentence = sentence[start + 1:]
    end = sentence.find('*')
    if end != -1:
        sentence = sentence[:end]

    for ch in sentence:
        cs ^= ord(ch)
    return f"{cs:02X}"


def verify_checksum(sentence: str) -> bool:
    """Verify if the NMEA sentence has a valid checksum."""
    sentence = sentence.strip()
    star_idx = sentence.find('*')
    if star_idx == -1 or star_idx + 3 > len(sentence):
        return False
    expected = sentence[star_idx + 1:star_idx + 3].upper()
    actual = compute_checksum(sentence[:star_idx])
    return expected == actual


def parse_lk8ex1(raw_line: str) -> Optional[Dict[str, Any]]:
    """
    Parse a raw LK8EX1 line into a dictionary with verified values.
    Returns None if line is not a valid LK8EX1 sentence.
    """
    line = raw_line.strip()
    if not line.startswith("$LK8EX1"):
        return None

    valid_cs = verify_checksum(line)

    # Extract body before checksum
    star_idx = line.find('*')
    body = line[1:star_idx] if star_idx != -1 else line[1:]
    parts = body.split(',')

    # Expected: ['LK8EX1', '101325', '99999', '15', '21', '999']
    if len(parts) < 6:
        return None

    try:
        pressure_pa = int(parts[1]) if parts[1] else 0
        raw_altitude = int(parts[2]) if parts[2] else 99999
        vario_cm_s = int(parts[3]) if parts[3] else 0
        temperature_c = float(parts[4]) if parts[4] else 0.0
        battery = int(parts[5]) if parts[5] else 999

        # Convert units
        pressure_hpa = pressure_pa / 100.0
        vario_m_s = vario_cm_s / 100.0

        # Barometric standard altitude: 44330 * (1 - (P / P0)^(1/5.255))
        # P0 = 101325 Pa standard sea level
        if pressure_pa > 30000:
            computed_altitude_m = 44330.0 * (1.0 - math.pow(pressure_pa / 101325.0, 0.19029495))
        else:
            computed_altitude_m = 0.0

        return {
            "raw": line,
            "valid_checksum": valid_cs,
            "pressure_pa": pressure_pa,
            "pressure_hpa": pressure_hpa,
            "raw_altitude": raw_altitude,
            "computed_altitude_m": computed_altitude_m,
            "vario_cm_s": vario_cm_s,
            "vario_m_s": vario_m_s,
            "temperature_c": temperature_c,
            "battery": battery,
        }
    except ValueError:
        return None


def format_lk8ex1(pressure_pa: int,
                  vario_cm_s: int,
                  temperature_c: int = 21,
                  altitude_m: int = 99999,
                  battery: int = 999) -> str:
    """Construct a valid LK8EX1 string with checksum."""
    body = f"LK8EX1,{pressure_pa},{altitude_m},{vario_cm_s},{temperature_c},{battery},"
    cs = compute_checksum(body)
    return f"${body}*{cs}\r\n"
