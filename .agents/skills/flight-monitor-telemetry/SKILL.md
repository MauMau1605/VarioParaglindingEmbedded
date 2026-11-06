---
name: flight-monitor-telemetry
description: >-
  Procedures for using and testing the Python flight monitor tools (serial telemetry, LK8EX1 parsing, TCP bridge for XCSoar/VarioAppli, and NMEA flight log replay). Use when testing telemetry streams, validating NMEA/LK8EX1 output, or running the flight monitor.
---

# Flight Monitor & Telemetry Skill

This skill provides procedures for running and testing the Python-based telemetry monitor, flight recorder, and TCP bridge located in `tools/vario_monitor/`.

## Key Capabilities

1. **Serial & BLE Telemetry Display:** Decodes `$LK8EX1` sentences (pressure, altitude, vario, temp, battery) at 10 Hz / 50 Hz.
2. **Built-in Flight Simulator:** Generates synthetic climb (+2.5 m/s) and sink (-1.8 m/s) profiles with realistic pressure variations.
3. **TCP Bridge (Port 8888):** Broadcasts real-time NMEA frames to external apps (XCSoar, LK8000, Android `VarioAppli`).
4. **Flight Logger & Replay:** Logs to `.nmea` and `.csv` in `tools/vario_monitor/logs/`.

---

## Usage Commands

### 1. Launch the Desktop Monitor GUI
```powershell
cd tools\vario_monitor
py main.py
```
*(Or execute `tools\vario_monitor\run_monitor.bat`)*

### 2. Verify LK8EX1 Sentence Format & Checksum
The sentence emitted by the firmware and parsed by the monitor has the form:
```text
$LK8EX1,pressure,altitude,vario,temperature,battery*checksum
```
* Example: `$LK8EX1,99540,154,12,23,4100*41\r\n`
* `pressure`: Pascals (e.g. `99540` = 995.40 hPa)
* `altitude`: Meters MSL (e.g. `154`)
* `vario`: Vertical climb/sink in cm/s (e.g. `12` = +0.12 m/s; negative for sink)
* `temperature`: Degrees Celsius (or `9999` if not available)
* `battery`: Battery voltage in mV or percentage (e.g. `4100` = 4.10V)
* `checksum`: Two-character hex XOR of all characters between `$` and `*`

### 3. Replay Existing Flight Logs for Testing
Inspect recorded logs in `tools/vario_monitor/logs/` (e.g. `flight_log_20260919_132802.nmea`) to validate parser edge cases and flight state transitions (ground $\rightarrow$ flight $\rightarrow$ landing).

