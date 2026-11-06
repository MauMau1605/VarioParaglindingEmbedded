---
name: embedded-doc-writer
description: >-
  Formats register bitmask tables, Mermaid sequence and architectural diagrams, Q-format mathematical formulas, and hardware wiring documentation. Use when creating or updating technical documentation, specs, or hardware guides.
---

# Embedded Documentation Skill

This skill standardizes technical documentation for hardware registers, system architecture, fixed-point math, and pinout wiring across the project.

## Documentation Standards

### 1. Register Maps & Bitmask Tables
When documenting sensor registers (BMP390, SAM-M10Q, or nRF peripherals), use structured tables with bit positions, access type, default values, and operational descriptions:

| Bit(s) | Field Name | Type | Reset | Description |
|:---:|:---|:---:|:---:|:---|
| `[7:4]` | `RESERVED` | R | `0x0` | Reserved; do not write. |
| `[3:2]` | `OSRS_P` | R/W | `0x0` | Pressure oversampling: `00`=1x, `01`=2x, `10`=4x, `11`=8x |
| `[1:0]` | `MODE` | R/W | `0x0` | Power mode: `00`=Sleep, `11`=Normal |

### 2. Architecture & Sequence Diagrams (Mermaid)
- **Layer Architecture:** Use `graph TD` or `flowchart TD` to display unidirectional dependencies (`common` $\rightarrow$ `hal` $\rightarrow$ `drivers` $\rightarrow$ `middleware` $\rightarrow$ `app`).
- **Timing & Event Sequences:** Use `sequenceDiagram` to visualize the 50 Hz timer loop, DMA interrupts, and BLE GATT notifications.
  - Quote all labels containing special characters: `id["Label (50 Hz)"]`.
  - Avoid HTML tags inside Mermaid labels.

### 3. Fixed-Point Mathematics Documentation
Always specify:
- Representation format (e.g. Q16.16, integer milli-units, cm/s).
- Scaling multipliers ($2^N$ or $10^K$).
- Intermediate type sizes (`int64_t`) and rounding strategies (e.g. truncation vs `+ (1 << (N-1))` rounding).

### 4. Hardware Wiring & Pinout Tables
Follow the format established in `docs/hardware_wiring.md`:
- MCU Pin (e.g., `P0.15`)
- Net Name (e.g., `I2C_SCL`)
- Direction (`Input`, `Output`, `I/O`, `Analog In`)
- Connected Peripheral / External Component (e.g., `BMP390 SCK + 4.7 kΩ pull-up`)
- Peripheral Channel (e.g., `TWIM0 / I2C0`)

