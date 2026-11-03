# ZonalECU-FWv2 Coding Rules & Guidelines

This document details the code formatting conventions, header structure requirements, comment standards, and documentation guidelines for the Zonal ECU Firmware v2 project.

---

## 1. General Principles
- **Language**: All source code comments, header file documentation, function descriptions, and CMake files must be strictly in **English**.
- **Cross-Compatibility**: All public headers must support both C and C++ callers seamlessly using `HEADER_CPP` and `FOOTER_CPP`.
- **Vertical Compactness**: Minimize wasted vertical space. Keep comment block heights low to avoid unnecessary scrolling.

---

## 2. Header File Structure

Every header file (`.h`) must follow this standardized section layout:

```c
/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __<MODULE>_<NAME>_H__
#define __<MODULE>_<NAME>_H__

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include "Macros.h"
/* Other inclusions */

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
HEADER_CPP

/* DEFINITIONS **********************************************************************************************
 ************************************************************************************************************/
/* Enums, typedefs, structs, macros */

/* SHARED FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/
/* Function prototypes */

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
FOOTER_CPP

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __<MODULE>_<NAME>_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
```

### 2.1 C / C++ Compatibility Pairings
- **Header Files (`.h`)**: `HEADER_CPP` and `FOOTER_CPP` **MUST** be used in pairs.
  - `HEADER_CPP` expands to `extern "C" {` when compiled with C++.
  - `FOOTER_CPP` expands to `}` when compiled with C++.
- **Source Files (`.c`)**: `HEADER_CPP` and `FOOTER_CPP` are **NOT** required in `.c` files since `.c` files are compiled with pure C compiler (`xtensa-esp32-elf-gcc`).

### 2.2 Header Inclusion Path
- Public headers must configure their component `CMakeLists.txt` so that callers can include headers either way:
  ```c
  #include "Common/Error.h"   /* Full module-qualified path */
  #include "Error.h"          /* Direct header inclusion */
  ```

---

## 3. Comment & Documentation Standards

### 3.1 Section Separator Banners
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

### 3.2 In-Code Grouping Comments
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

### 3.3 Comment Block Height & Doxygen Tagging (Compact Documentation)
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

### 3.4 Function Documentation
- Use compact Doxygen tags with parameter direction qualifiers:
  ```c
  /** @brief Returns constant string representation of a zecu_err_t error code
   *  @param err Error code
   *  @return const char* Static string representation of the error name */
  const char * ZECU_ErrToStr(IN zecu_err_t err);

  /** @brief Copies error string name into user-provided buffer
   *  @param err Error code
   *  @param str Destination character buffer (minimum 32 bytes)
   *  @return ZECU_OK on success, ZECU_ERR_INVALID_ARG if str is NULL */
  zecu_err_t ZECU_Err2Str(IN zecu_err_t err, OUT char * str);
  ```

---

## 4. Parameter & Permission Macros

Defined in `components/Common/Macros.h`:
- `RO`: `const` (Read-Only permission).
- `WO`: Write-Only permission.
- `RW`: Read/Write permission.
- `IN`: `const` (Input argument).
- `OUT`: Output argument (pointer).
- `INOUT`: Input/Output argument (pointer).

---

## 5. Error Code Conventions (`zecu_err_t`)

- Error codes are typed as `zecu_err_t` (defined in `components/Common/Error.h`).
- Standard ESP-IDF errors are mapped 1:1 (`ESP_OK`, `ESP_FAIL`, `ESP_ERR_NO_MEM`, ...).
- Subsystem base error codes are allocated in 0x100 increments:
  - `0x1000`: Communication base (`ZECU_ERR_BASE_COMM`).
  - `0x1100`: W5500 SPI Ethernet base (`ZECU_ERR_BASE_W5500`).
  - `0x1200`: CAN / CAN-FD base (`ZECU_ERR_BASE_CAN`).
  - `0x1300`: Storage / NVS base (`ZECU_ERR_BASE_STORAGE`).
  - `0x1400`: GPIO / ADC / IO base (`ZECU_ERR_BASE_IO`).

