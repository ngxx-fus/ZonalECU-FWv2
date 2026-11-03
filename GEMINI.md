# ZonalECU-FWv2 Project Rules & Guidelines

This document persists all project architectural rules, coding standards, and build configuration guidelines.

---

## 1. Interaction & Workflow Rules
- **Rule Confirmation (Meta-Rule)**: After each user request, if any new rule, project convention, or architectural pattern is established or modified, the assistant must ask the user for confirmation before appending/updating it in `GEMINI.md`.
- **Language Convention**: All source code comments, header file documentation, and CMake scripts must be written strictly in **English**.

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

## 4. Code & Header Standards
- **Header Guards**: `#ifndef __<MODULE>_<NAME>_H__` with `HEADER_CPP` / `FOOTER_CPP` for C++ linkage compatibility.
- **Header Inclusion**: Public headers include parent folder in `INCLUDE_DIRS` so callers can use either `#include "Module/Header.h"` or `#include "Header.h"`.
- **Toolchain & LSP**: `.clangd` is maintained at project root pointing to `build/compile_commands.json` with Xtensa GCC query-driver configured for exact autocompletion and diagnostic accuracy.

---

## 5. Comment Standards

### 5.1 Section Separator Banners
- **Major Section Separators**: Use 2-line asterisk banners with total column width $W = 108$ characters:
  ```c
  /* SHARED FUNCTIONS *****************************************************************************************
   ************************************************************************************************************/
  ```
- **Sub-Section Separators**: When sub-sections are necessary, use a single-line asterisk banner aligned to the exact same column width ($W = 108$ characters):
  ```c
  /* SUB-SECTION NAME *****************************************************************************************/
  ```
- **Prohibited Separator Formats (DON'T)**: Never use equal-sign boxed borders or custom divider lines:
  ```c
  /* =========================================================================
   * Prohibited Format
   * ========================================================================= */
  ```

### 5.2 In-Code Grouping Comments
- For grouping enum fields, struct members, or local code blocks, use **plain text-only** comments without any separator characters or decorative lines:
  ```c
  /* Success & Generic Status (ESP_OK, ESP_FAIL) */
  ZECU_OK                             = ESP_OK,                  /*!< 0x000: Success / No error */
  ZECU_SUCCESS                        = ESP_OK,                  /*!< 0x000: Success alias */
  ZECU_FAIL                           = ESP_FAIL,                /*!< 0x1  : Generic failure */

  /* Standard ESP-IDF System Error Codes (Mapped from ESP_ERR_*) */
  ZECU_ERR_NO_MEM                     = ESP_ERR_NO_MEM,          /*!< 0x101: Out of memory */
  ZECU_ERR_INVALID_ARG                = ESP_ERR_INVALID_ARG,     /*!< 0x102: Invalid argument */
  ```

### 5.3 Comment Block Height & Doxygen Tagging (Compact Documentation)
- Keep comment block heights as compact as possible to minimize vertical scrolling:
  - Open documentation directly on the first line with `/** @brief ...` (double-asterisk to activate Doxygen/Clangd LSP tooltip parsing) and close immediately on the last line with `... */`.
  - Avoid standalone `/**` opening and `*/` closing lines.
- Multi-part or extended comments must use explicit Doxygen tags:
  - Use `@brief <summary>` on the first line.
  - Follow with `@details <details>` or `@note <note>` for additional explanation. Omit any unnecessary or irrelevant tags.
  - Do not use loose, untagged continuation lines directly beneath `@brief`.

**DON'T**:
```c
/**
 * @brief Standard Zonal ECU Error Code definitions (zecu_err_t)
 * Mapped 1:1 with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals.
 */
```
*or single-asterisk / untagged continuation line (ignored by Doxygen/Clangd LSP):*
```c
/* @brief Standard Zonal ECU Error Code definitions (zecu_err_t)
 * Mapped 1:1 with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals. */
```

**DO**:
```c
/** @brief Standard Zonal ECU Error Code definitions (zecu_err_t)
 *  @note  Mapped 1:1 with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals. */
```
