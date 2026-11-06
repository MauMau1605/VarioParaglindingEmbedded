---
name: memory-footprint-audit
description: >-
  Analyzes RAM and Flash footprint on nRF52833, checks section boundaries (128 KB RAM, 512 KB Flash), verifies zero heap allocation, and identifies large static symbols. Use when checking binary size or optimizing memory footprint.
---

# Memory Footprint Audit Skill

This skill analyzes and verifies the static memory layout of the firmware to ensure it operates strictly within the **nRF52833** hardware boundaries.

## Target Hardware Limits

* **Flash (ROM):** 512 KB (`0x00080000` bytes)
* **RAM (SRAM):** 128 KB (`0x00020000` bytes)
* **Heap Allocated:** **0 KB** (`CONFIG_HEAP_MEM_POOL_SIZE=0`)
* **Main Thread Stack:** 2048 bytes (`CONFIG_MAIN_STACK_SIZE=2048`)
* **System Workqueue Stack:** 1024 bytes (`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=1024`)

---

## Memory Audit Commands

From the project root (after running a build):

### 1. Generate RAM Utilization Report
```powershell
ninja -C build ram_report
```
*Outputs detailed breakdown of BSS, DATA, and noinit sections per object file.*

### 2. Generate ROM / Flash Utilization Report
```powershell
ninja -C build rom_report
```
*Outputs detailed breakdown of TEXT and RODATA sections per object file.*

### 3. Quick Size Summary
```powershell
ninja -C build size
```

---

## Inspection Guidelines

1. **Zero Heap Verification:**
   Verify `CONFIG_HEAP_MEM_POOL_SIZE=0` in `.config` inside `build/zephyr/.config`. If any component attempts to call `k_malloc` or heap functions, it must fail at link or runtime.
2. **Buffer Sizing:**
   Ensure large buffers (such as GNSS UART ring buffer, BLE transmission queues) are statically allocated with compile-time bounded sizes (`std::array<T, N>`).
3. **Stack Guard:**
   Ensure deep recursion is completely absent. Any function allocating more than 128 bytes on stack should be refactored to use static or persistent buffers.
4. **Symbol Audit:**
   Search `build/zephyr/zephyr.map` for unexpected symbols exceeding 512 bytes:
   ```powershell
   Get-Content build\zephyr\zephyr.map | Select-String -Pattern "0x[0-9a-f]{8}\s+0x[0-9a-f]{3,}"
   ```

