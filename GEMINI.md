# Antigravity Standalone Agent Guidelines

All code, comments, documentation, and agent reasoning must be in English.

## Standalone Orchestration Directives
1. The default entry point is the **Orchestrator** defined in `.antigravity/config.json`.
2. Antigravity acts as a standalone autonomous orchestrator: analyze the user prompt, break it down, and delegate execution to the specialized sub-agents in `.antigravity/prompts/` (Architect, DriverDev, Tester, DocWriter, Reviewer) using their assigned models without manual switching.
3. Every generated code output must be audited against the **Reviewer** checklist before completing the task.

---
## 1. System Target & Core Constraints

- **Platform:** Embedded system — **Seeed XIAO SAMD21** (ATSAMD21G18A, ARM Cortex-M0+).
- **Build System:** PlatformIO with Arduino framework.
- **Resource Constraints:** 32 KB RAM, 256 KB Flash. Minimal footprint, cache-friendly data layouts, deterministic execution timing.
- **Execution Model:** Single-threaded cooperative loop (`setup()` / `loop()`). No RTOS.
- **Strictly Prohibited:**
  - **No Dynamic Memory Allocation:** Never use `malloc`, `free`, `new`, or `delete`. Do not use heap-allocating STL containers (`std::vector`, `std::string`, `std::map`, `std::list`).
  - **No C++ Exceptions:** Compile with `-fno-exceptions`. Never throw or catch exceptions.
  - **No C++ Templates / Generic Programming:** No complex template meta-programming, template classes, or heavy generic header libraries. Keep abstractions concrete or interface-driven via function pointers/lightweight virtual interfaces to prevent code bloat.
  - **No RTTI:** Compile with `-fno-rtti`. Avoid `dynamic_cast` and `typeid`.
  - **No Floating Point:** The Cortex-M0+ has no FPU. Use fixed-point integer arithmetic exclusively.

---

## 2. Model Routing Matrix

Select or dispatch tasks to models based on complexity and risk profile:

| Stage / Task Nature | Recommended Model | Selection Justification |
|---|---|---|
| **System architecture, memory layout planning, peripheral layer abstraction** | Claude Opus 4 (Thinking) | Uncompromising architectural rigor; detects hidden coupling and design anti-patterns. |
| **Hardware driver debugging, timing analysis, ISR safety audits** | Claude Sonnet 4 (Thinking) | Step-by-step reasoning essential for hardware timing, volatile registers, and concurrency hazards. |
| **Concrete driver & module implementation (`.hpp`/`.cpp`), register bitmasking** | Claude Sonnet 4 | High code readability, idiomatic low-overhead C++, exact adherence to project style. |
| **Full codebase dependency checks, peripheral map auditing, memory map reviews** | Gemini 3 Pro | Massive context window allows parsing entire SDK headers and project files simultaneously. |
| **Boilerplate register definitions, unit test runners, build flags/PlatformIO adjustments** | Gemini 3 Flash | Instant response latency and minimal token usage for deterministic scaffolding. |

---

## 3. Architectural Rules & Dependency Boundaries

1. **Strict Layer Isolation (Golden Rule):**
   - Structural hierarchy (bottom → top):
     `common/` → `hal/` → `drivers/` → `middleware/` → `app/`
   - **Zero Upward Dependencies:** A lower layer must **never** include a higher-layer header or invoke higher-layer functions directly.
   - For bottom-up notifications (e.g., driver interrupts, timer events):
     - Use lightweight C-style function pointers with an explicit `void* user_data` context.
     - Never use `std::function` (avoids heap allocation).

2. **No Circular Dependencies:**
   - Keep include chains unidirectional.
   - Use forward declarations for structs/classes whenever pointers or references suffice.
   - Keep header files (`.hpp`) strictly minimal: expose only public API signatures. Keep internal state, registers, and constants hidden inside `.cpp` translation units.

---

## 4. C++ Embedded Coding Standards

1. **Short, Single-Purpose Functions:**
   - Target function length: **under 20–25 lines**.
   - Functions must do exactly one thing (Single Responsibility Principle). Decompose complex hardware initialization or parsing pipelines into private, well-named static sub-routines.

2. **Memory & Buffer Management:**
   - Use compile-time static allocations or stack memory with controlled maximum depth.
   - For fixed-size buffers, use `std::array<T, N>` or fixed-size C arrays.
   - Use non-owning views such as `std::string_view` or non-owning slice structures for read-only buffer inspection without copying.
   - Check stack limits carefully—never allocate large buffers on the stack (SAMD21 has only 32 KB RAM total).

3. **Error Handling:**
   - Return explicit status codes using `enum class Error : uint8_t` defined in `common/error_codes.hpp`.
   - Always check return values of peripheral and driver calls. Never silently discard an error code.

4. **Types & Hardware Correctness:**
   - Always use fixed-width integers from `<cstdint>` (`uint8_t`, `int16_t`, `uint32_t`, etc.). Do not use bare `int`, `long`, or `short`.
   - Mark memory-mapped registers, hardware buffers, and variables shared with ISRs explicitly as `volatile`.
   - Enforce const-correctness everywhere: if a pointer, reference, or local value is not modified, it must be `const`.

5. **Naming Conventions:**
   - Classes & Structs: `PascalCase`
   - Functions & Methods: `snake_case`
   - Constants & Macros: `kPascalCase` (or `UPPER_SNAKE_CASE` for hardware register masks)
   - Private/Protected Member Variables: prefixed with `m_` (e.g., `m_rx_buffer`)

---

## 5. Agent Verification Checklist

Before emitting or modifying any C++ code, the agent must verify:
- [ ] No `new`, `delete`, `malloc`, or `free` are present.
- [ ] No templates (`template <...>`) or STL dynamic containers are introduced.
- [ ] No `try`, `catch`, or `throw` statements are used.
- [ ] No floating-point (`float`, `double`) operations are used.
- [ ] Lower-layer files do not include or reference higher-layer headers.
- [ ] All functions remain concise, focused, and under ~25 lines.