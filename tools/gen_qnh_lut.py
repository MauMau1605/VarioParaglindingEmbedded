#!/usr/bin/env python3
"""Generate QNH scale lookup table for barometric altitude calculations."""

H = 44330.0
vals = [round(65536 * (1 - (-500 + 100 * i) / H) ** -5.255) for i in range(46)]
for i in range(0, 46, 6):
    print("    " + ", ".join(f"{v}U" for v in vals[i:i + 6]) + ",")

