---
name: embedded-driver-dev
description: >-
  Guidelines and design patterns for developing bare-metal and HAL/driver code for nRF52833 and Zephyr RTOS (I2C/TWIM, UART EasyDMA, SAADC, PWM). Use when writing or refactoring hardware drivers, HAL facades, or fixed-point sensor routines.
---

# Embedded Driver & HAL Development Skill

This skill governs the implementation of hardware drivers (`drivers/`) and hardware abstraction layers (`hal/`) for the **nRF52833** target running Zephyr RTOS.

## Core Design Patterns

### 1. Concrete Structs / Classes (No Virtual Tables / No Templates)
- Do not use abstract base classes with virtual methods (`virtual void init() = 0;`) to avoid vtable RAM overhead and RTTI.
- Do not use C++ template classes. Keep driver abstractions concrete.

### 2. Upward Event Callbacks (No `std::function`)
When a driver or HAL needs to notify an upper layer (e.g. UART DMA receive complete, timer tick):
```cpp
// Correct: C-style function pointer + context
typedef void (*UartRxCallback)(const uint8_t* data, size_t length, void* user_data);

void set_rx_callback(UartRxCallback cb, void* user_data);
```
Never use `std::function` or lambda captures (avoids heap allocation).

### 3. Error Handling
Every hardware operation that may fail (I2C NACK, bus timeout, invalid checksum) must return an `enum class Error : uint8_t` from `common/error_codes.hpp`:
```cpp
Error init();
Error read_registers(uint8_t reg, uint8_t* buffer, size_t len);
```
Never discard return values. Handle or propagate errors explicitly.

### 4. Memory-Mapped IO & DMA Buffers
- Mark buffers shared with hardware DMA (e.g. UARTE EasyDMA RX/TX buffers) as `volatile` or manage cache-coherency if needed.
- Place DMA buffers in RAM (never flash/const) and align them if required by the hardware:
```cpp
static uint8_t s_dma_rx_buf[kBufferSize] __attribute__((aligned(4)));
```

### 5. Fixed-Point Arithmetic Pattern
- All physical units are scaled to integers (e.g. pressure in Pa, temperature in 0.01 °C, altitude in cm, climb rate in cm/s).
- Use 64-bit integer intermediates (`int64_t`) to prevent arithmetic overflow prior to division or bit-shifting:
```cpp
// Example: Fixed-point climb rate calculation in cm/s
int32_t delta_p = current_p - previous_p; // in Pa
int32_t vario_cm_s = static_cast<int32_t>((static_cast<int64_t>(delta_p) * kScaleFactor) >> kShiftBits);
```

### 6. Function Length Rule
- Keep all functions **strictly under 20–25 lines**.
- Break down register configuration pipelines into private static helper functions:
  - `configure_registers()`
  - `verify_chip_id()`
  - `load_calibration_coefficients()`

