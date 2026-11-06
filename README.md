# VarioGPS-BLE — BLE Variometer & GNSS Flight Tracker

[![Target](https://img.shields.io/badge/MCU-Nordic%20nRF52833-blue.svg)](https://www.nordicsemi.com/Products/nRF52833)
[![RTOS](https://img.shields.io/badge/RTOS-Zephyr%20%2F%20NCS-brightgreen.svg)](https://www.zephyrproject.org/)
[![Barometer](https://img.shields.io/badge/Baro-Bosch%20BMP390-orange.svg)](https://www.bosch-sensortec.com/)
[![GNSS](https://img.shields.io/badge/GNSS-u--blox%20SAM--M10Q-red.svg)](https://www.u-blox.com/)
[![Language](https://img.shields.io/badge/C%2B%2B-17%20(Embedded%2C%20No%20Heap)-lightgrey.svg)]()

**VarioGPS-BLE** is an ultra-compact, high-precision solar-chargeable variometer and GNSS flight tracker designed for paragliding, hang gliding, and sailplanes. Built on the **Nordic nRF52833** SoC running **Zephyr RTOS**, it combines a **Bosch BMP390** pressure sensor and a **u-blox SAM-M10Q** GNSS module with differential audio tone feedback and real-time **Bluetooth Low Energy (BLE)** telemetry streaming to mobile navigation apps (**XCTrack**, **XCSoar**, **LK8000**).

---

## Key Features

- **High Precision Barometer:** Bosch BMP390 acquired at **50 Hz** over I2C (address `0x76`) with x8 pressure oversampling and on-chip IIR filtering.
- **Fast Low-Latency Variometer:** Real-time vertical speed filtered via discrete integer single-pole IIR ($\tau \approx 187$ ms).
- **High-Efficiency GNSS:** u-blox SAM-M10Q module streaming `UBX-NAV-PVT` at **1 Hz** over UART using **EasyDMA double buffering** and power-saving cyclic tracking (PSMCT).
- **Intelligent Flight FSM:** Automatic 3-stage state machine (`WAIT_GPS_FIX` $\rightarrow$ `PRE_TAKEOFF_CALIB` $\rightarrow$ `IN_FLIGHT`):
  - Automatically derives and locks barometric reference pressure (QNH) from 20-sample rolling GPS altitude on the ground using an integer Flash lookup table.
  - Automatically freezes QNH upon launch and captures pre-trigger flight points.
- **Differential Audio Variometer:** Passive piezo buzzer driven by differential PWM across two GPIOs ($2 \times V_{\text{DD}} \approx 6.6\text{ V}_{\text{p-p}}$) with guaranteed zero DC bias on silence.
- **Battery & Power Management:** High-impedance SAADC monitoring of LiPo battery voltage with piecewise linear SoC estimation; DC/DC REG1 active; CPU enters System ON `WFE` sleep for $>97\%$ of each 20 ms cycle.
- **Multi-Protocol BLE Telemetry:**
  - Compact binary Vario notification (14 bytes, 10 Hz)
  - Compact binary GPS notification (28 bytes, 1 Hz)
  - Standard ASCII **`$LK8EX1`** NMEA notification stream
  - Battery level percentage and device status notifications
  - Runtime configurable streaming settings via GATT write
- **Strict Embedded Architecture:**
  - Zero dynamic heap allocation (`no malloc`, `no new`).
  - Pure 32-bit and 64-bit fixed-point integer mathematics (no `float` or `double`).
  - No C++ exceptions or RTTI (`-fno-exceptions -fno-rtti`).
  - Strict hierarchical layers (`common/` $\rightarrow$ `hal/` $\rightarrow$ `drivers/` $\rightarrow$ `middleware/` $\rightarrow$ `app/`).
  - All functions under 25 lines.

---

## Hardware Specification

| Peripheral | Board Net / Pins | Configuration |
|---|---|---|
| **MCU** | nRF52833-QDAA | Cortex-M4F @ 64 MHz, 128 KB RAM, 512 KB Flash |
| **BMP390** | `P0.15` (SCL), `P0.17` (SDA) | I2C Fast Mode (400 kHz), Address `0x76` |
| **SAM-M10Q** | `P0.04` (TX), `P0.30` (RX) | UART EasyDMA 115200 bps, UBX binary only |
| **Piezo Buzzer** | `P0.02` (BUZ1), `P0.03` (BUZ2) | Differential PWM, Normal/Inverted polarity |
| **Battery ADC** | `P0.05` (AIN3) | 4.7 MΩ / 4.7 MΩ divider, 40 µs acquisition time |
| **Antenna** | Walsin RFANT3216120A5T | 2.4 GHz ceramic chip antenna |

For full pinouts, schematics, and electrical details, see [docs/hardware_wiring.md](docs/hardware_wiring.md).

---

## Build & Flash Instructions

### Prerequisites
- [nRF Connect SDK](https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK) (v2.9.0 or compatible)
- [West](https://docs.zephyrproject.org/latest/develop/west/index.html) and CMake toolchains

### Building the Firmware

```bash
# Initialize workspace with pinned manifest
west init -m https://github.com/<your-repo>/VarioGPS-BLE
west update

# Build for custom target board overlay
west build -b nrf52833dk_nrf52833 -- -DEXTRA_DTC_OVERLAY_FILE=boards/vario_tracker.overlay

# Flash via J-Link SWD probe
west flash
```

---

## Repository Structure

```text
VarioGPS-BLE/
├── boards/               # Devicetree overlays and board configurations
├── docs/
│   ├── architecture.md   # Layered architecture, mathematical reference, and GATT specification
│   └── hardware_wiring.md# Electrical pinout, schematics, and hardware guide
├── src/
│   ├── app/              # Application orchestrator (50 Hz periodic loop & sleep)
│   ├── common/           # Compile-time constants, error codes, and shared types
│   ├── drivers/          # BMP390 and SAM-M10Q UBX binary drivers
│   ├── hal/              # Zephyr HALs (I2C, UART Async EasyDMA, PWM, ADC, BLE)
│   ├── middleware/       # Vario filter, QNH LUT, buzzer acoustics, battery monitor, BLE service
│   └── main.cpp          # Zephyr main() application entry point
├── tools/
│   └── vario_monitor/    # Python PC monitor GUI, flight recorder, and TCP bridge
├── CMakeLists.txt        # Zephyr application CMake configuration
├── prj.conf              # Project Kconfig configuration
├── Kconfig               # Top-level application Kconfig
├── west.yml              # West manifest
└── README.md             # Project documentation
```

---

## Technical Documentation
- [Software Architecture & Fixed-Point Mathematics](docs/architecture.md)
- [Hardware Wiring & Pinout Guide](docs/hardware_wiring.md)
