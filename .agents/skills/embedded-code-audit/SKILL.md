---
name: embedded-code-audit
description: >-
  Audits C++ code changes for strict compliance with embedded constraints: zero heap allocation, no floating point, no C++ templates or exceptions, function length under 25 lines, and layer boundary isolation. Use before completing any code modification or when asked to review/audit code.
---

# Embedded Code Audit & Compliance Skill

This skill enforces strict gatekeeping rules for all firmware code on the **nRF52833-QDAA** target.

## Audit Checklist (Must Pass All Before Completion)

Whenever code in `src/` is created or modified, execute this audit checklist:

### 1. Zero Dynamic Allocation (Heap)
- [ ] No occurrences of `malloc`, `free`, `realloc`, `calloc`.
- [ ] No `new`, `delete`, `new[]`, `delete[]`.
- [ ] No heap-allocating STL containers (`std::vector`, `std::string`, `std::map`, `std::list`, `std::deque`, `std::queue`).
- [ ] No smart pointers that manage heap memory (`std::unique_ptr`, `std::shared_ptr`, `std::make_unique`, `std::make_shared`).
- [ ] Only static allocations, `std::array<T, N>`, or fixed-size C arrays are permitted.

### 2. Zero Floating-Point Arithmetic
- [ ] No primitive float types: `float`, `double`, `long double`.
- [ ] No floating-point math headers: `<cmath>`, `<math.h>`.
- [ ] All conversions (barometer pressure, altitude, vario climb rate, battery voltage) must strictly use **fixed-point integer arithmetic** (Q-format or integer scaling like milli-volts, cm/s, Pa).

### 3. Language & Runtime Feature Restrictions
- [ ] No C++ templates (`template <...>`). Use concrete types or C-style function pointers with context.
- [ ] No C++ exceptions: no `try`, `catch`, or `throw` (compiled with `-fno-exceptions`).
- [ ] No RTTI: no `dynamic_cast`, no `typeid` (compiled with `-fno-rtti`).
- [ ] No `std::function` (which can allocate on the heap). Use explicit function pointers: `void (*callback)(void* context)`.

### 4. Layer Isolation & Dependency Direction (Golden Rule)
- [ ] Direction: `common/` $\rightarrow$ `hal/` $\rightarrow$ `drivers/` $\rightarrow$ `middleware/` $\rightarrow$ `app/`.
- [ ] **Zero upward dependencies:**
  - `common/` must never include anything outside `common/`.
  - `hal/` must never include `drivers/`, `middleware/`, or `app/`.
  - `drivers/` must never include `middleware/` or `app/`.
  - `middleware/` must never include `app/`.
- [ ] Upward notifications must use callbacks with `void* user_data`.

### 5. Function Size & Cleanliness
- [ ] Target function length: **strictly under 20–25 lines**.
- [ ] Single Responsibility Principle: decompose complex initialization, parsing, or math into well-named private `static` helper routines.

### 6. Embedded Typing & Hardware Correctness
- [ ] Fixed-width integers from `<cstdint>` only (`uint8_t`, `int16_t`, `uint32_t`, `int64_t`). No raw `int`, `long`, `short`.
- [ ] Member variables prefixed with `m_` (e.g. `m_state`, `m_buffer`).
- [ ] Constants use `kPascalCase` or `UPPER_SNAKE_CASE` for register bitmasks.
- [ ] Variables shared with ISRs or DMA hardware buffers marked `volatile`.
- [ ] Error handling returns `enum class Error : uint8_t` from `common/error_codes.hpp`. Always check return codes.

