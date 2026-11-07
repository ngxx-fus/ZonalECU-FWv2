# STM32 Coding Rules & Guidelines (FW-STM)

This document details core architectural rules, hardware targets, build configuration, code formatting conventions, header structure requirements, naming standards (aligned with the **Renesas RA FSP** coding style), comment rules, and defensive embedded C guidelines for the STM32F103C8T6 real-time sub-firmware.

---

## 1. Project Architecture & Hardware Target
- **Target Chip**: STM32F103C8T6 (ARM Cortex-M3 32-bit RISC, 72 MHz clock frequency, 64 KB Flash, 20 KB SRAM).
- **Core Role**: Real-time deterministic sub-controller for sensor acquisition, low-latency actuator drives, and vehicle Classic CAN node communication.
- **Toolchain**: GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`) with CMake or GNU Make.
- **Hardware Abstraction**: STM32Cube HAL / LL (Low-Layer) drivers & CMSIS.
- **Deterministic Real-Time Constraint**:
  - Interrupt Service Routines (ISRs) must execute within bounded, deterministic time (sub-microsecond).
  - Heavy processing, floating point calculations, and non-critical logic must be deferred to main loop or FreeRTOS worker tasks.
  - Watchdog timer (IWDG) must be periodically refreshed.
- **Language Convention**: All source code comments, header file documentation, function descriptions, and build scripts must be written strictly in **English**.

---

## 2. General Principles
- **Language & Spell Check**: All source code comments, identifiers, header file documentation, function descriptions, and build files must be strictly in **English**.
- **Cross-Compatibility**: All public headers must support both C and C++ callers seamlessly:
  ```c
  #ifdef __cplusplus
  extern "C" {
  #endif
  ...
  #ifdef __cplusplus
  }
  #endif
  ```
- **Indentation (`tabsize = 4`)**: Always indent using **4 spaces** per level (including nested preprocessor macros). Never use raw `\t` tab characters.
- **Vertical Compactness**: Minimize wasted vertical space. Keep comment block heights low and functions concise to avoid unnecessary scrolling.
- **Line Width**: No line of code or documentation may exceed **$W = 108$ characters** (up to 110 columns for standard section banners).
- **Layering**: Respect strict architectural layering (Application $→$ Middleware/CAN Protocol $→$ Drivers/HAL $→$ CMSIS/Registers). Layers must communicate only via well-defined interfaces.
- **Header Guards & Namespaces**:
  - Every header file must protect against multiple inclusion using `__<MODULE>_<NAME>_H__`.
  - Generic headers (such as `Config.h`, `Types.h`) must be qualified by their module namespace to prevent naming collisions.

---

## 3. File Structure & Header Conventions

### 3.1 Standardized Header File Layout
Every header file (`.h`) must follow this standardized section layout:

```c
/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __<MODULE>_<NAME>_H__
#define __<MODULE>_<NAME>_H__

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/

/* DEFINITIONS **********************************************************************************************
 ************************************************************************************************************/

/* EXPORTED VARIABLES ***************************************************************************************
 ************************************************************************************************************/

/* SHARED VARIABLES *****************************************************************************************
 ************************************************************************************************************/

/* SHARED FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
#ifdef __cplusplus
}
#endif

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __<MODULE>_<NAME>_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
```

---

## 4. Naming Conventions (Renesas RA FSP Aligned)

| Identifier Category | Format / Convention | Examples |
| :--- | :--- | :--- |
| **Constants & Macros** | `UPPER_SNAKE_CASE` | `STM32_CAN_BAUDRATE_500K`, `MAX_BUFFER_SIZE` |
| **Typedefs / Structs / Enums** | `snake_case_t` | `stm32_can_msg_t`, `subecu_state_t` |
| **Public API Functions** | `PascalCase` with module prefix | `Stm32_Can_Init()`, `Stm32_Gpio_SetOutput()` |
| **Static / Private Functions** | `prv_<Module>_<Action>()` | `prv_Can_ProcessFifo()`, `prv_Adc_FilterSample()` |
| **Variables (Local)** | `camelCase` | `rxMessage`, `sampleCount`, `timeoutMs` |
| **Global / Shared Variables** | `g_<module><Name>` | `g_canRxBuffer`, `g_systemState` |

---

## 5. Defensive Programming & Safety Guidelines

1. **Parameter Validation**: Validate all incoming pointers and range-checked parameters at function entry:
   ```c
   if (pMsg == NULL) {
       return STM32_ERR_NULL_PTR;
   }
   ```
2. **Volatile Keyword**: Variables shared between ISRs and background code MUST be marked `volatile`:
   ```c
   static volatile bool g_canFrameReceived = false;
   ```
3. **Deterministic Memory**: Dynamic memory allocation (`malloc`, `free`) is **strictly forbidden** in `FW-STM`. All memory must be statically or stack-allocated.
4. **Hardware Access**: Never write naked magic numbers to peripheral registers. Always use CMSIS/HAL bitmask constants.
