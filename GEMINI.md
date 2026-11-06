# ZonalECU-FWv2 Project Rules & Guidelines

This document persists core project architectural rules, hardware targets, build configuration, and workflow guidelines.

> [!IMPORTANT]
> Detailed source code formatting, header layout conventions, and comment standards are maintained in [CodingRules.md](CodingRules.md).

---

## 1. Interaction & Workflow Rules
- **Rule Confirmation (Meta-Rule)**: After each user request, if any new rule, project convention, or architectural pattern is established or modified, the assistant must ask the user for confirmation before appending/updating it in `GEMINI.md` or `CodingRules.md`.
- **Structural Change Confirmation (Mandatory)**: Whenever any structural change is proposed — including file/directory reorganization, moving/renaming/merging files, code architecture modifications, or restructuring documentation (e.g. merging, splitting, or modifying section hierarchies) — the assistant **MUST** explicitly explain the intended structural changes and ask for user confirmation BEFORE making any edits.
- **Language Convention**: All source code comments, header file documentation, and CMake scripts must be written strictly in **English**.
- **Rule Reference**: Specific code-level styling and comment conventions must align with [CodingRules.md](CodingRules.md).

---

## 2. Hardware & Build Configuration
- **Target Chip**: ESP32 (Xtensa architecture).
- **CPU Specifications**: Dual-Core (`CONFIG_FREERTOS_NUMBER_OF_CORES=2`), running at **240 MHz** (`CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y`).
- **Wireless Interfaces**: Completely **disabled** (No Wi-Fi, No Bluetooth) to minimize power consumption, reduce binary size, and dedicate maximum bus/CPU bandwidth to ECU tasks.
- **Compiler Optimization**: High performance `-O2` (`CONFIG_COMPILER_OPTIMIZATION_PERF=y`).
- **Binary Footprint**: Assertions silent (`CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT=y`), check macros silent (`CONFIG_COMPILER_OPTIMIZATION_CHECKS_SILENT=y`), default log level `WARN` to save flash and IRAM.

---

## 3. Project & Component Architecture
- **Modular Component Structure (Leaf-Level Components)**:
  - No monolithic `components/CMakeLists.txt` at the root of `components/`.
  - Every individual module under `components/` (e.g. `components/Common`, `components/comm/w5500`) has its own leaf-level `CMakeLists.txt`.
  - The root `CMakeLists.txt` uses `file(GLOB_RECURSE ...)` to auto-discover all leaf component directories and append them to `EXTRA_COMPONENT_DIRS` (ignoring `.template/`).
- **Template System**:
  - `components/.template/` serves as the blueprint for creating any new module/driver.
- **Common Module (`components/Common`)**:
  - `Macros.h`: Contains C/C++ cross-compatibility macros (`HEADER_CPP`, `FOOTER_CPP`) and argument attribute macros (`IN`, `OUT`, `INOUT`, `RO`, `WO`, `RW`).
  - `Error.h`: Defines `zecu_err_t`, mapped 1:1 to `ESP_ERR_*` and extended with subsystem base ranges (`ZECU_ERR_BASE_*`).
  - `Atomic.h`: Typedefs for atomic operations.
  - `SharedAPIs.h`: Single include header aggregating all common definitions.

---

## 4. Toolchain & IDE Configuration
- **Toolchain & LSP**: `.clangd` is maintained at project root pointing to `build/compile_commands.json` with Xtensa GCC query-driver configured for exact autocompletion and diagnostic accuracy.
- **Code Standards**: Refer to [CodingRules.md](CodingRules.md) for full specifications on headers, section banners, grouping comments, and compact Doxygen tags.
