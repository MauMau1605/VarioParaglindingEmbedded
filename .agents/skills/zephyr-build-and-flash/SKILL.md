---
name: zephyr-build-and-flash
description: >-
  Builds, cleans, flashes, and inspects Zephyr RTOS firmware for nRF52833-QDAA using West, CMake, and J-Link/pyOCD runners. Use when building, flashing, debugging, or verifying compilation of the firmware.
---

# Zephyr Build and Flash Skill

This skill provides verified, reproducible commands for building, compiling, and flashing the **VarioGPS-BLE** firmware on the **nRF52833-QDAA** target using Zephyr RTOS and West.

## Target & Board Configuration

* **Target Board:** `nrf52833dk/nrf52833`
* **Custom Board Overlay:** `boards/vario_tracker.overlay`
* **Project Config:** `prj.conf` (with `boards/vario_tracker_defconfig` if building standalone board target)

---

## Standard Build Commands

### 1. Incremental Build
Use for normal development cycles:
```powershell
west build -b nrf52833dk/nrf52833 -s . -- -DDTC_OVERLAY_FILE="boards/vario_tracker.overlay"
```

### 2. Pristine Build (Clean Rebuild)
Use when CMakeLists.txt, Kconfig, or devicetree overlays have changed:
```powershell
west build -p always -b nrf52833dk/nrf52833 -s . -- -DDTC_OVERLAY_FILE="boards/vario_tracker.overlay"
```

### 3. Clean Build Artifacts
```powershell
west build -t clean
```
Or remove the `build/` directory completely if CMake state is corrupted.

---

## Flashing & Debugging

### 1. Flash via SEGGER J-Link
```powershell
west flash --runner jlink
```

### 2. Flash via pyOCD (Alternative SWD Runner)
```powershell
west flash --runner pyocd
```

### 3. View RTT Real-Time Logs
Logging is configured over Segger RTT (`CONFIG_USE_SEGGER_RTT=y`, `CONFIG_LOG_BACKEND_RTT=y`).
Open J-Link RTT Viewer or run:
```powershell
JLinkRTTClient
```

---

## Compiler Warnings & Strictness Verification

The build enforces:
```cmake
target_compile_options(app PRIVATE
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
    -Wall -Wextra -Werror
    -Wno-unused-parameter
)
```
**Zero warnings are allowed:** Any unused variable, implicit truncation, or unhandled enum value will trigger `-Werror` and fail the build.

