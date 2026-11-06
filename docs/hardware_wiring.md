# Hardware Wiring & Pinout Guide: nRF52833 VarioGPS-BLE

This document describes the electrical wiring, pin mappings, and hardware specifications for the custom **nRF52833-QDAA** board integrating the **Bosch BMP390** barometric pressure sensor, **u-blox SAM-M10Q** GNSS module, differential piezo buzzer, and battery monitoring circuit.

---

## 1. MCU & Peripheral Overview

| Subsystem | Component | Interface & Pins | Operational Details |
|:---|:---|:---|:---|
| **Microcontroller** | Nordic nRF52833-QDAA | ARM Cortex-M4F, 128 KB RAM, 512 KB Flash | 64 MHz, DC/DC REG1 enabled, LFXO (32.768 kHz) + HFXO (32 MHz) |
| **Barometer** | Bosch Sensortec BMP390 | I2C (SCL=P0.15, SDA=P0.17) | 7-bit Address `0x76` (SDO=GND), 400 kHz fast mode, external 4.7 kΩ pull-ups |
| **GNSS Module** | u-blox SAM-M10Q | UART (TX=P0.04, RX=P0.30) | Boots at 9600 bps; auto-configured to 115200 bps UBX-only; 1 Hz NAV-PVT |
| **Acoustic Audio** | Passive Piezo Buzzer | Differential PWM (P0.02, P0.03) | Driven in phase opposition; series 150 Ω resistors; zero DC bias on silence |
| **Battery ADC** | Voltage Divider (4.7 MΩ / 4.7 MΩ) | SAADC AIN3 (P0.05) | Internal 0.6V reference, gain 1/4, 40 µs acquisition time, 12-bit resolution |
| **RF / BLE** | Walsin RFANT3216120A5T | 2.4 GHz Ceramic Antenna | Matching network, +8 dBm to -20 dBm configurable TX power |

---

## 2. Complete Pin Assignment Table

```text
                           nRF52833-QDAA (QFN40)
                               +-----------+
    (Piezo +) BUZ1_MCU [P0.02] | 1      40 | P0.00 (LFXO 32.768 kHz)
    (Piezo -) BUZ2_MCU [P0.03] | 2      39 | P0.01 (LFXO 32.768 kHz)
  (UART TX) MCU_TX_P004 [P0.04] | 3      38 | P0.31 (Unused)
      (ADC)   VBAT_ADC [P0.05] | 4      37 | P0.30 [MCU_RX_P030] (UART RX)
                               | ...       |
      (I2C SCL) I2C_SCL [P0.15] | 15     26 | XC2 (32 MHz Crystal)
      (I2C SDA) I2C_SDA [P0.17] | 17     25 | XC1 (32 MHz Crystal)
                               +-----------+
```

| MCU Pin | Net Name | Direction | Connected Peripheral & Component | Function |
|:---:|:---|:---:|:---|:---|
| **P0.02** | `BUZ1_MCU` | Output | R8 (150 Ω) $\rightarrow$ J5.1 (Piezo terminal 1) | PWM Channel 0 (Normal polarity) |
| **P0.03** | `BUZ2_MCU` | Output | R9 (150 Ω) $\rightarrow$ J5.2 (Piezo terminal 2) | PWM Channel 1 (Inverted polarity) |
| **P0.04** | `MCU_TX_P004` | Output | R13 (150 Ω) $\rightarrow$ SAM-M10Q RXD | UART1 TX (EasyDMA) |
| **P0.05** | `VBAT_ADC` | Analog In | 4.7 MΩ / 4.7 MΩ divider from `VBAT` | SAADC Channel AIN3 |
| **P0.15** | `I2C_SCL` | I/O | BMP390 SCK + 4.7 kΩ pull-up to 3.3V | I2C0 SCL (400 kHz) |
| **P0.17** | `I2C_SDA` | I/O | BMP390 SDI + 4.7 kΩ pull-up to 3.3V | I2C0 SDA (400 kHz) |
| **P0.30** | `MCU_RX_P030` | Input | R14 (150 Ω) $\leftarrow$ SAM-M10Q TXD | UART1 RX (EasyDMA) |
| **P0.00** | `XL1` | Analog In | 32.768 kHz crystal oscillator | LFXO Input |
| **P0.01** | `XL2` | Analog Out| 32.768 kHz crystal oscillator | LFXO Output |
| **XC1** | `XC1` | Analog In | 32 MHz crystal oscillator | HFXO Input |
| **XC2** | `XC2` | Analog Out| 32 MHz crystal oscillator | HFXO Output |
| **Pin 19** | `SWDIO` | I/O | SWD Programming / Debug Pad | Debug Serial Wire Data |
| **Pin 20** | `SWDCLK` | Input | SWD Programming / Debug Pad | Debug Serial Wire Clock |

---

## 3. Peripheral Schematics & Interconnects

### 3.1 BMP390 Barometric Sensor
- **I2C Address:** SDO is connected to `GND`, setting the 7-bit slave address to **`0x76`**.
- **Pull-ups:** Dual external 4.7 kΩ pull-up resistors are installed on `SCL` (P0.15) and `SDA` (P0.17) to `3.3V_SYS`.
- **Interrupt:** `INT` is not connected; sampling is strictly periodic at 50 Hz via firmware timer.

### 3.2 u-blox SAM-M10Q GNSS Module
- **Power:** Powered from `3.3V_SYS`. No separate backup battery; `V_BCKP` is tied to `3.3V_SYS` (hot start preserved while board is powered).
- **Communication:** UART at 3.3V CMOS logic through 150 Ω damping resistors.
- **Protocol:** Firmware configures the receiver to UBX binary output exclusively at 115200 baud, disabling ASCII NMEA strings to minimize CPU overhead.

### 3.3 Differential Piezo Buzzer
- **Differential Drive:** Pins `P0.02` and `P0.03` toggle $180^\circ$ out of phase during tone output, doubling the effective acoustic voltage across the piezo to $2 \times V_{\text{DD}} \approx 6.6\text{ V}_{\text{p-p}}$.
- **Zero DC Bias:** When audio is silenced, both channels are held at 0V (ground level). Firmware never leaves one pin high while the other is low.

### 3.4 Battery Voltage Divider & SAADC
- **Voltage Divider:** $R_{11} = 4.7\text{ M}\Omega$ (top) and $R_{12} = 4.7\text{ M}\Omega$ (bottom).
- **Division Ratio:** $V_{\text{pin}} = V_{\text{bat}} / 2$.
- **Source Impedance:** $R_{\text{eff}} \approx 2.35\text{ M}\Omega$. The SAADC acquisition time is configured to **40 µs** to allow the internal sampling capacitor to fully settle.

---

## 4. Hardware Constraints & Design Rules

1. **User Interface:** The board has no buttons, LEDs, or display. Boot confirmation and errors are reported via distinct audio tones through the piezo buzzer.
2. **System Reset:** System reset occurs via power cycle switch (SW1) or software `NVIC_SystemReset()`.
3. **Firmware Programming:** Flashing is performed via SWD header (`SWDIO`, `SWDCLK`, `3.3V`, `GND`) using a J-Link probe or nRF52 DK debug out.
