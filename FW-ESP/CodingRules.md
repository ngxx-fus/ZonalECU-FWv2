# Coding Rules & Project Guidelines

This document details core project architectural rules, hardware targets, build configuration, code formatting conventions, header structure requirements, naming standards (aligned with the **Renesas RA FSP** coding style), comment rules, and defensive embedded C guidelines for the Zonal ECU Firmware v2 project.

---

## 1. Project Architecture & Hardware Target
- **Target Chip**: ESP32 (Xtensa architecture, dual-core running at 240 MHz with `-O2` performance optimization).
- **Wireless Interfaces**: Completely disabled (No Wi-Fi, No Bluetooth) to minimize power consumption and dedicate bus/CPU bandwidth to ECU tasks.
- **Binary Footprint**: Assertions silent (`CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_SILENT=y`), check macros silent, default log level `WARN` to save flash and IRAM.
- **Modular Component Structure (Leaf Components)**:
  - Component root directory is `comp/` (documentation directory is `doc/`).
  - Every individual module under `comp/` (e.g. `comp/Common`, `comp/Comm/W5500`) maintains its own independent `CMakeLists.txt`.
  - The root `CMakeLists.txt` uses recursive scanning (`file(GLOB_RECURSE ...)`) to automatically discover all leaf component directories under `comp/` into `EXTRA_COMPONENT_DIRS`, automatically ignoring the `.template/` directory.
- **ESP-IDF Wrapper Directory Naming**:
  - Any directory wrapping or adapting ESP-IDF drivers/subsystems **MUST** start with the prefix `ESP-` (e.g. `ESP-SPI`, `ESP-FreeRTOS`, `ESP-UART`).
- **Common Module (`comp/Common`)**:
  - `Config/Macros.h`: C/C++ cross-compatibility macros (`HEADER_CPP`, `FOOTER_CPP`) and argument attribute macros (`IN`, `OUT`, `INOUT`, `RO`, `WO`, `RW`).
  - `Error/Error.h`: Standard error codes (`ze_err_t`) mapped 1:1 to `ESP_ERR_*` and extended with subsystem base ranges (`ZE_ERR_BASE_*`).
  - `Atomic/Atomic.h`: Typedefs for atomic operations.
  - `SharedAPIs.h`: Single umbrella header aggregating all common definitions.
- **Language Convention**: All source code comments, header file documentation, function descriptions, and CMake scripts must be written strictly in **English**.

---

## 2. General Principles
- **Language & Spell Check**: All source code comments, identifiers, header file documentation, function descriptions, and CMake files must be strictly in **English** and pass automated spell checking (`codespell` / `cSpell`). Domain-specific acronyms (`ZECU`, `SOMEIP`, `NVS`, etc.) are maintained in `.vscode/settings.json` (`cSpell.words`).
- **Cross-Compatibility**: All public headers must support both C and C++ callers seamlessly using `HEADER_CPP` and `FOOTER_CPP`.
- **Indentation (`tabsize = 4`)**: Always indent using **4 spaces** per level (including nested preprocessor macros). Never use raw `\t` tab characters.
- **Vertical Compactness**: Minimize wasted vertical space. Keep comment block heights low and functions concise (ideally within a single screen/page) to avoid unnecessary scrolling.
- **Line Width**: No line of code or documentation may exceed **$W = 128..130$ characters** (up to 130 columns for standard section banners).
- **Layering**: Respect strict architectural layering (Application $→$ Middleware/Services $→$ Drivers $→$ ESP-IDF/HAL). Layers must communicate only with adjacent layers via well-defined interfaces. Any performance-critical layer bypass must be explicitly documented.
- **Header Namespace & Anti-Collision Rule**:
  - For common or shared headers across modules (e.g. `Shared_APIs.h`, `Config.h`, `Types.h`), consumers **MUST** include them using their module namespace prefix:
    - Layer-qualified (most specific): `#include "App/ESP-FreeRTOS/Shared_APIs.h"` or `#include "Comm/ESP-ClassicCAN/Shared_APIs.h"`.
    - Component-qualified: `#include "ESP-FreeRTOS/Shared_APIs.h"` or `#include "ESP-ClassicCAN/Shared_APIs.h"`.
  - Bare inclusion of generic headers without a namespace prefix (e.g. `#include "Shared_APIs.h"`) is **STRICTLY FORBIDDEN** to prevent header shadowing and collision across components.
  - Globally unique headers (such as `Error.h`, `Macros.h`, `Atomic.h` in `comp/Common`) may be included directly.

---

## 2. File Structure & Header Conventions

### 2.1 File Extensions & Header Guards
- Use `.h` for header files and `.c` for C source files.
- Every header file must protect against multiple inclusion using the project's standardized guard format: `__<MODULE>_<NAME>_H__`.
- **No Data Definitions in Header Files**: Never define or allocate variables in `.h` files (e.g., `int g_var = 0;`). Define variables once in a `.c` file and declare them `extern` in the header only when global access is unavoidable.

### 2.2 Standardized Header File Layout
Every header file (`.h`) must follow this standardized section layout:

```c
/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __<MODULE>_<NAME>_H__
#define __<MODULE>_<NAME>_H__

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include "Macros.h"
/* Other inclusions (group by subsystem and document non-obvious includes) */

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
HEADER_CPP

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/
/* Constant definitions and function-like macros */

/* DEFINITIONS **********************************************************************************************
 ************************************************************************************************************/
/* Enums, typedefs, structs */

/* EXPORTED VARIABLES ***************************************************************************************
 ************************************************************************************************************/
/* extern declarations (avoid if possible) */

/* SHARED FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/
/* Public function prototypes */

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
FOOTER_CPP

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __<MODULE>_<NAME>_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
```

### 2.3 C / C++ Compatibility Pairings
- **Header Files (`.h`)**: `HEADER_CPP` and `FOOTER_CPP` **MUST** be used in pairs.
  - `HEADER_CPP` expands to `extern "C" {` when compiled with C++.
  - `FOOTER_CPP` expands to `}` when compiled with C++.
- **Source Files (`.c`)**: `HEADER_CPP` and `FOOTER_CPP` are **NOT** required in `.c` files since `.c` files are compiled with the pure C compiler (`xtensa-esp32-elf-gcc`).

### 2.4 Header Inclusion Path
- Public headers must configure their component `CMakeLists.txt` so that callers can include headers either way:

```c
#include "Common/Error.h"   /* Full module-qualified path */
#include "Error.h"          /* Direct header inclusion */
```

---

## 3. Naming Conventions (Renesas RA FSP Style)

Names must clearly express intent, scope, and type role. Structs and types are **nouns**; functions are **verbs** performing actions.

### 3.1 Summary of Naming Patterns

| Category | Convention | Prefix / Suffix | Examples |
| :--- | :--- | :--- | :--- |
| **Functions (General / Public)** | `<PREFIX>_<PascalCase>()` | `UPPER_` or `Pascal_` prefix + `PascalCase` | `ETH_Open()`, `ZE_ErrToStr()`, `Comm_Init()` |
| **Private Functions** | `prv_<snake_case>()` / `prv_<PascalCase>()` | `prv_` or `__` lowercase prefix | `prv_update_time()`, `prv_reset_phy()` |
| **Local Variables** | `snake_case` | None (or unit suffix) | `retry_cnt`, `timeout_ms`, `frame_idx` |
| **Pointer Variables** | `p_<snake_case>` | `p_` (or `pp_` for double ptr) | `p_ctrl`, `p_cfg`, `p_buf`, `pp_head` |
| **Global Variables** | `g_<snake_case>` | `g_` (or `gp_` for global ptr) | `g_eth0_ctrl`, `gp_active_log` |
| **Static File Variables** | `s_<snake_case>` or `g_` | `s_` / `sp_` (or `g_` / `gp_`) | `s_is_initialized`, `sp_rx_ring` |
| **Common & Module Types (`typedef`)** | `<module>_<name>_t` | Lowercase `<module>_` + `_t` suffix | `ze_err_t`, `eth_ctrl_t`, `eth_cfg_t`, `w5500_speed_t` |
| **Macros & `#define`** | `UPPER_SNAKE_CASE` | `<MODULE>_` prefix | `ZE_MASK32()`, `ETH_MAX_PKT_BYTES` |
| **Enum Labels** | `UPPER_SNAKE_CASE` | `<MODULE>_` prefix | `ZE_OK`, `ETH_STATE_OPEN` |
| **Global Constants** | `UPPER_SNAKE_CASE` | `<MODULE>_` prefix | `ETH_DEFAULT_TIMEOUT_MS` |

### 3.2 Function Names (`PascalCase`)
- **PascalCase Function Naming (Mandatory)**: Functions **SHALL** use `PascalCase` for the function body name, preceded by the module/visibility prefix:
  - **Public API Functions**: Use an **UPPERCASE** subsystem/driver prefix (`ZE_`, `SOMEIP_`, `ETH_`, `SPI_`, `CAN_`, `W5500_`) or a **PascalCase** module prefix (`Comm_`, `I2c_`, `App_`, `UserApp_`), followed by `_` and a `PascalCase` verb phrase:

```c
ze_err_t ETH_Open(INOUT eth_ctrl_t * p_ctrl, IN eth_cfg_t * p_cfg);
const char * ZE_ErrToStr(IN ze_err_t err);
ze_err_t Comm_SendPacket(IN comm_ctrl_t * p_ctrl, IN uint8_t * p_data, IN size_t data_len);
```

  - **Private (File-Scope `static`) Functions**: Use a **lowercase** prefix (`prv_` or `__`) separated by `_` (either `prv_<snake_case>()` or `prv_<PascalCase>()`):

```c
static void prv_update_time(void);
static ze_err_t prv_validate_config(IN eth_cfg_t * p_cfg);
```

- **Action-Oriented**: Every function performs an action, so its name must state *what* it does using a verb phrase (e.g., `ETH_CheckForErrors()` instead of `ETH_ErrorCheck()`, `App_DumpDataToFile()` instead of `App_DataFile()`).
- **Standard Function Prefixes**:
  - `is_` / `Is`: Boolean query returning `bool` (`true` / `false`), e.g., `prv_is_hit_retry_limit()`, `ETH_IsLinkUp()`.
  - `get_` / `Get`: Retrieve a value or attribute, e.g., `ETH_GetMacAddr()`.
  - `set_` / `Set`: Configure or update a value, e.g., `ETH_SetBaudRate()`.

### 3.3 Variables & Pointers
- **Stack / Local Variables**: Use `snake_case` (all lowercase with `_` word separators) so variable scope is immediately distinguishable from functions and macros:

```c
ze_err_t err       = ZE_OK;
uint32_t   retry_cnt = 0U;
```

- **Pointer Variables (`p_`, `pp_`)**:
  - In strict alignment with the **Renesas RA FSP** convention, all pointer variables and pointer parameters **MUST** be prefixed with `p_` (or `pp_` for pointer-to-pointer).
  - Place the `*` separated by spaces (`type_t * p_name`) or adjacent to the variable name (`type_t *p_name`), and never declare multiple variables on the same line:

```c
eth_ctrl_t * p_ctrl = NULL;
char       * p_str  = NULL;
uint8_t   ** pp_buf = NULL;
```
- **Global & Module-Scope Variables (`g_`, `gp_`, `s_`, `sp_`)**:
  - Avoid global variables whenever possible.
  - When required, prepend global variables with `g_` (and global pointers with `gp_`). File-scope `static` variables should use `s_` / `sp_` (or `g_` / `gp_` for FSP instance structures):

```c
eth_ctrl_t   g_eth0_ctrl;
logger_t   * gp_system_log = NULL;
static bool  s_is_driver_ready = false;
```

- **Include Units in Variable Names**:
  - Whenever a variable or constant represents time, frequency, size, weight, voltage, or another physical quantity, append the unit suffix (`_ms`, `_us`, `_s`, `_hz`, `_bytes`, `_mv`, etc.):

```c
uint32_t timeout_ms;
uint32_t spi_clock_hz;
size_t   payload_bytes;
```

- **Standard Variable Suffixes**:
  - `_max`: Maximum allowed value (e.g., `retry_max`, `frame_len_max`).
  - `_min`: Minimum allowed value (e.g., `voltage_mv_min`).
  - `_cnt`: Current running count of a counter variable (e.g., `retry_cnt`, `rx_err_cnt`).
  - `_len` / `_size`: Element count or byte capacity (e.g., `header_len`, `ring_buf_size`).
  - `_idx`: Loop or array index (e.g., `slot_idx`).
  - `_key`: Lookup or identifier key (e.g., `session_key`).

### 3.4 Common Types (`<module>_<name>_t`), Structures & Enums
- **Common & Module Type Naming (`<module>_<name>_t` — Mandatory)**:
  - All custom types (`typedef` of structs, enums, unions, function pointers, or primitive aliases) **SHALL** use lowercase `snake_case` in the format `<module>_<name>_t` (ending with `_t`):

```c
/* DON'T: PascalCase or unprefixed custom typedefs */
typedef uint32_t         AtomicUint32_t;
typedef struct EthCfg    EthCfg_t;
/* DO: <module>_<name>_t in lowercase snake_case */
typedef _Atomic uint32_t ze_atomic_u32_t;
typedef enum ze_err_e    ze_err_t;
typedef struct st_eth_cfg eth_cfg_t;
```

  - Driver and module types should follow Renesas FSP `<module>_<name>_t` role naming:
    - `<module>_cfg_t`: Read-only configuration structure passed to `Open()` / `Init()`.
    - `<module>_ctrl_t`: Runtime control block / state structure managed by the driver.
    - `<module>_cb_args_t`: Callback argument structure.
    - `<module>_api_t` / `<module>_instance_t`: Polymorphic interface table and instance descriptor.
  - Struct tags (when used) follow `st_<module>_<name>` and enum tags follow `e_<module>_<name>`:

```c
typedef struct st_eth_cfg {
    uint8_t const * p_mac_addr;     /*!< 6-byte hardware MAC address */
    uint32_t        timeout_ms;
    uint16_t        rx_buf_size;
    uint8_t         spi_host_id;
    bool            is_full_duplex;
} eth_cfg_t;
```

- **Structure Member Layout & Alignment**:
  - Group structure fields logically and order them by size (64-bit / 32-bit / pointers first, then 16-bit, then 8-bit / `bool`) to prevent wasted RAM from compiler alignment padding.
  - Declare one member per line. Omit inline comments when the member name is already self-explanatory; add `/*!< ... */` only when extra context or constraints are needed.
  - Avoid C bitfields for hardware registers or network/bus frames due to compiler-dependent ordering; use explicit bit-mask and shift macros instead.
- **Enumerations (`enum`)**:
  - **Use `enum` for Groups of Constants (Mandatory)**: Any related group of integer constants, states, register offsets, or command IDs **SHALL** be defined using `enum` instead of `#define` macros so that symbolic names are preserved for the compiler and debugger, and without consuming RAM like `const` variables:

```c
/* DON'T: Using #define macros for a group of related constants */
#define ETH_SPEED_10M       (0U)
#define ETH_SPEED_100M      (1U)
#define ETH_SPEED_1000M     (2U)
/* DO: Using enum for a group of related constants */
typedef enum e_eth_speed {
    ETH_SPEED_10M   = 0,
    ETH_SPEED_100M  = 1,
    ETH_SPEED_1000M = 2,
} eth_speed_t;
```

  - Enum labels must be `UPPER_SNAKE_CASE` prefixed with the module name.
  - For state enums, include an explicit uninitialized, closed, or error state as the first label (e.g., `ETH_STATE_CLOSED = 0`, `ETH_STATE_OPEN`, `ETH_STATE_ERR`).
  - **Size Caveat**: Because `enum` storage size is implementation-defined, never use an `enum` type directly as a field inside packed wire protocols, CAN frames, or NVS binary blobs; store the value in an explicit fixed-width integer field (`uint8_t`, `uint16_t`, `uint32_t`).

### 3.5 Macros & Constants
- **Scope of `#define` Constants**: Use `#define` only for standalone compile-time configuration switches (used with `#if`), bit-manipulation/utility expressions, or non-integer literals. Any group of related integer constants **SHALL** use `enum` instead (see Section 3.4).
- **Naming**: All `#define` macros and global `const` constants must use `UPPER_SNAKE_CASE` and start with a module prefix (`ZE_`, `ZECU_`, `ETH_`, etc.) to prevent namespace collisions. Avoid generic unprefixed names like `MAX` or `MIN`.
- **Type-Qualifier Macro Exceptions & Consistency Rule**: The permission and pointer annotation macros in `Macros.h` are available in 4 equivalent styles: Uppercase (`RO`, `PTR_RO`), Lowercase (`ro`, `ptr_ro`), Leading `__` Uppercase (`__RO`, `__PTR_RO`), and Leading `__` Lowercase (`__ro`, `__ptr_ro`). Developers may choose any style freely, but **MUST use strictly ONE single style per source/header file** (never mix different casing or underscore prefix styles within the same file).
- **Parenthesization**: Always wrap every macro parameter and the entire macro expression in parentheses:

```c
#define ZE_ADD(x, y)            ((x) + (y))
#define ZE_MASK32(bit_pos)      (1UL << (bit_pos))
```

- **Multi-Statement Macros**: Wrap multi-statement macros in `do { ... } while (0)` so trailing semicolons behave safely inside `if/else` statements, and right-align continuation backslashes (`\`):

```c
#define ZE_SET_PAIR(v, w, x, y) \
    do {                        \
        (v) = (x) + (y);        \
        (w) = (y) + 2;          \
    } while (0)
```

- **Inline Functions vs. Macros**: Prefer `static inline` functions over function-like macros whenever possible to gain type checking and avoid double-evaluation side effects (such as `MAX(f(x), z++)`).
- **No Syntax Aliasing**: Never use macros to disguise C syntax or keywords.

---

## 4. Code Formatting & Control Flow

### 4.1 Indentation & Tab Size (`tabsize = 4`, Including Macros)
- **4-Space Indentation (No Tabs)**: All code blocks, struct/enum members, and continuation blocks **SHALL** use **4 spaces** per indentation level (`tabsize = 4`). Raw tab characters (`\t`) are strictly prohibited.
- **Macro & Preprocessor Indentation**: Nested preprocessor directives (`#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, and `#define`) **SHALL** also be indented by **4 spaces** per nesting level (excluding the outer file header guard `__<MODULE>_<NAME>_H__`):

```c
#ifdef __cplusplus
    #ifndef HEADER_CPP
        #define HEADER_CPP extern "C" {
    #endif
#else
    #ifndef HEADER_CPP
        #define HEADER_CPP
    #endif
#endif
```

- **Multi-Line Macro Bodies**: Inside `do { ... } while (0)` macro definitions, indent each level by 4 spaces.

### 4.2 Braces `{}` Policy
- **Always Use Braces**: Every `if`, `else if`, `else`, `for`, `while`, and `do` statement must use `{}` braces, even when the body is a single statement:

```c
/* Check if the operation failed */
if (ZECU_OK != err) {
    /* Propagate error code to caller */
    return err;
}
```

- **Closing Brace Comments**: For long or deeply nested blocks, annotate the closing brace to clarify which block is ending:

```c
} /* end while (retry_cnt < retry_max) */
```

### 4.3 Parentheses `()` & Spacing Policy
- Put a single space between control-flow keywords and the opening parenthesis: `if (...)`, `while (...)`, `for (...)`, `switch (...)`.
- Do **not** put a space between a function identifier and its parameter list: `ETH_Open(p_ctrl, p_cfg);`.
- Do **not** wrap `return` expressions in unnecessary parentheses:

```c
return ZECU_OK; /* DO */
return (ZECU_OK); /* DON'T */
```

### 4.4 One Statement & One Variable Per Line
- Write at most **one statement per line**.
- Declare **one variable per line**, and **always initialize local variables at declaration**:

```c
/* DON'T */
char * p_src, * p_dst;
/* DO */
char * p_src = NULL;
char * p_dst = NULL;
```

### 4.5 Conditional Expressions & Comparisons
- **Constant on Left-Hand Side (Yoda Conditions)**: Place constants, literals, `NULL`, or enum labels on the left-hand side of `==` and `!=` comparisons so accidental omission of `=` is caught by the compiler and the target state is immediately visible:

```c
if (NULL == p_ctrl) {
    return ZECU_ERR_INVALID_ARG;
}
if (ZECU_OK == err) {
    /* Handle success */
}
```

- **Explicit Non-Zero / Null Comparisons**: Do not rely on implicit non-zero evaluation for error codes, integers, or pointers. Only `bool` variables and `is_...` predicates should be tested without an explicit comparison operator:

```c
/* DON'T */
if (ETH_Init(p_ctrl)) { ... }
if (!(buf_size % sizeof(uint32_t))) { ... }
/* DO */
if (ZECU_OK != ETH_Init(p_ctrl)) { ... }
if (0U == (buf_size % sizeof(uint32_t))) { ... }
if (is_link_active) { ... }
```

- **Avoid Embedded Assignments**: Do not embed assignments inside `if` conditions or arithmetic expressions:

```c
/* DON'T */
if (ZECU_OK != (err = ETH_Open(p_ctrl, p_cfg))) { ... }
/* DO */
err = ETH_Open(p_ctrl, p_cfg);
if (ZECU_OK != err) { ... }
```

- **`if / else if / else` Completeness**: When chaining `if ... else if` branches, include a trailing `else` block to handle or log unexpected states.

### 4.6 `switch` Statement Formatting
- Always provide a `default:` branch. If `default:` should be unreachable in normal operation, return an error code or log a warning.
- Any intentional fall-through between `case` labels must be documented with an explicit `/* Fall through */` comment.
- If a `case` branch declares local variables, enclose the branch body in `{ ... }` braces:

```c
/* Dispatch action based on current Ethernet driver state */
switch (state) {
    case ETH_STATE_INIT:
        prv_prepare_clocks();
        /* Fall through */
    case ETH_STATE_OPEN:
    {
        zecu_err_t err = prv_start_mac(p_ctrl);
        /* Abort if MAC layer failed to start */
        if (ZECU_OK != err) {
            /* Return MAC startup error */
            return err;
        }
        /* Exit switch after MAC starts */
        break;
    }
    default:
        /* Reject unrecognized state */
        return ZECU_ERR_INVALID_STATE;
}
```

### 4.7 `goto`, `continue`, `break`, and Ternary `?:`
- **`goto`**: Avoid `goto` except for centralized resource cleanup (`goto cleanup;`) when exiting a function that allocates multiple resources or locks. Place the label alone on a left-aligned line.
- **`continue` and `break`**: Use sparingly. Never mix `continue` and `break` inside complex `while` loops where `continue` could accidentally bypass loop counter updates.
- **Ternary Operator (`?:`)**: Keep ternary expressions simple and wrap the condition in parentheses:

```c
uint32_t delay_ms = (is_fast_mode) ? FAST_DELAY_MS : NORMAL_DELAY_MS;
```

- **Document Null Loop Bodies**: If a `while` or `for` loop intentionally has an empty body, place `; /* Intentional null body */` inside braces on its own line.

---

## 5. Defensive Embedded C Practices

- **Const Correctness**: Always qualify read-only pointer arguments and static lookup tables with `const` (or the `IN` / `RO` macros).
- **No Magic Numbers**: Never embed raw numeric literals in logic. Replace them with descriptive `#define` macros or `enum` constants.
- **Floating-Point Restrictions**: Never use floating-point variables as loop counters or for discrete states. Never compare floats with `==` or `!=`; always use `<=` or `>=` with an epsilon bound.
- **Error Return Check Policy**: Check the `zecu_err_t` return code of every function call. If a return value is intentionally ignored (e.g., non-critical diagnostic output), explicitly cast the call to `(void)`.
- **Use `#if` Instead of `#ifdef` for Configuration Flags**:
  - Prefer `#if FEATURE_ENABLED` or `#if defined(FEATURE_ENABLED)` over `#ifdef FEATURE_ENABLED` so passing `-DFEATURE_ENABLED=0` disables the block as expected.
- **Disabling Code Blocks**:
  - Never leave bare `#if 0` blocks in the codebase. Use a descriptive macro name (`#if ZE_NOT_YET_IMPLEMENTED`, `#if ZE_TEMP_DISABLED`) paired with a comment explaining why and when it was disabled.
- **Zero-Footprint File Identification (Friendly Reminder: Say No to `__FILE__`! 🚫📁)**:
  - To protect our 2MB Flash and keep assertion/logging diagnostics ultra-lightweight, **avoid using raw `__FILE__`** (which dumps long absolute paths into `.rodata` and eats up valuable Flash).
  - All `.c` source files in `main/` and `comp/` are automatically hashed by CMake at configure/build time into a unique 16-bit identifier: **use `ZE_FILE_ID` instead**!
  - File ID to source path mappings are automatically maintained in [`main/SourceLists.txt`](main/SourceLists.txt). When decoded on a host or diagnostic tool, `ZE_FILE_ID` resolves instantly without costing a single byte of text Flash on the chip.


---

## 6. Comment & Documentation Standards

### 6.1 Section Separator Banners
- **Major Section Separators**: Use 2-line asterisk banners with total column width $W = 130$ characters:

```c
/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/
```

- **Sub-Section Separators**: When sub-sections are necessary, use a single-line asterisk banner aligned to the exact same column width ($W = 130$ characters):

```c
/* SUB-SECTION NAME **************************************************************************************************************/
```

- **Prohibited Separator Formats (DON'T)**: Never use equal-sign boxed borders or custom divider lines:

```c
/* =========================================================================
 * Prohibited Format
 * ========================================================================= */
```

### 6.2 Empty Line Before Comments & Branching Comments (Mandatory)
- **Empty Line Before Standalone Comments**: Following standard C style conventions (Linux Kernel / Google / BARR-C), always insert **one empty (blank) line before a standalone comment or multi-line comment block** so the comment visually binds to the code below it, not the statement above it.
  - **Exceptions** (no preceding blank line required):
    1. Line 1 of a file (`/* GUARD/BEGIN ...`).
    2. Immediately after an opening brace `{` (first line inside a function, struct, enum, or control block) or `case` / `default:` label.
    3. Consecutive comment lines belonging to the same comment group.
- **Always Comment Before Branching Statements**: Every branching or control-flow transfer statement—including `if`, `else if`, `else`, `switch`, `break`, `continue`, `return`, and `goto`—**MUST** be preceded by a concise comment explaining the condition, decision, or reason for branching:

```c
zecu_err_t ETH_PollLink(INOUT eth_ctrl_t * p_ctrl)
{
    zecu_err_t err = ZECU_OK;
    /* Validate control block pointer before accessing state */
    if (NULL == p_ctrl) {
        /* Immediately after '{': no blank line needed before this comment */
        return ZECU_ERR_INVALID_ARG;
    }
    /* Blank line above separates previous block from this comment + branch */
    if (p_ctrl->is_link_up) {
        prv_refresh_link_stats(p_ctrl);
    }
    /* Otherwise, trigger PHY auto-negotiation */
    else {
        err = prv_start_autoneg(p_ctrl);
        /* Exit early if PHY negotiation failed to start */
        if (ZECU_OK != err) {
            /* Propagate hardware error to caller */
            return err;
        }
    }
    /* Return success after link setup completes */
    return ZECU_OK;
}
```

### 6.3 Self-Documenting Identifiers (No Redundant Comments)
- **Omit Comments When the Name Is Self-Explanatory**: Do **not** add comments to variables, struct fields, functions, macros, or enum values if their identifier name already clearly conveys their purpose.
- Only write declaration/definition comments when additional information—such as hardware constraints, valid ranges, nontrivial algorithms, or side effects—cannot be expressed by the identifier name alone.

```c
/* DON'T: Redundant comments that merely repeat the identifier name */
uint32_t retry_max  = 5U;    /* Maximum retry count */
uint32_t timeout_ms = 100U;  /* Timeout in milliseconds */
#define  ETH_PORT_MAX (4U)   /* Max Ethernet ports */
/* DO: Let clear names speak for themselves; comment only non-obvious constraints */
uint32_t retry_max  = 5U;
uint32_t timeout_ms = 100U;  /* Must be >= 50 ms per W5500 PLL lock specification */
#define  ETH_PORT_MAX (4U)
```

### 6.4 Type Definitions (`enum`, `struct`, `union`, `class`) & Macro Alignment
> [!IMPORTANT]
> **Strict Context Separation (Isolation Principle)**:
> - Quy tắc định dạng cho **Types (`enum`, `struct`, `union`, `class`)** và **Macros** tại mục này là **hoàn toàn độc lập** với quy tắc tài liệu hóa **Functions & Multi-Param Macros** tại Mục 6.5 & 6.6.
> - Tuyệt đối không để các quy tắc căn lề Doxygen của hàm/macro làm thay đổi hoặc phá vỡ cấu trúc căn chỉnh dấu `=` và comment nội bộ của enum/struct/macro.

- **Sub-Section Divider Comments Inside Types (`enum`, `struct`, `union`, `class`)**:
  - For grouping fields into sub-sections or categories, use plain text-only comments: `/* Category Name */`.
  - **Mandatory Spacing**: Always leave **at least 01 empty line before and after** the divider comment to maintain visual separation between logical groups.
- **Detailed Multi-Line Block Comments Inside Types**:
  - Inside an enum, struct, or union, if a member requires in-depth documentation, place a Doxygen block (`/** @brief ... *  @note ... */`) **directly above** that member. Never leave extra blank lines between the Doxygen block and its target member.
  - Standard type-level or member-level header blocks with only `@brief` and `@note` maintain the standard Column 12 content alignment (`/** @brief ` and ` *  @note  `).
- **Enum Member Assignment (`=`) Alignment**:
  - All assignment operators (`=`) within the same `enum` definition **MUST be vertically aligned**.
  - The `=` column is aligned at `longest_member_name + 1 space`.
- **Trailing Comments (`/*!< ... */`) Alignment**:
  - Short member documentation on the same line must use Doxygen trailing format (`/*!< ... */`).
  - All trailing comments within the block **MUST be vertically aligned** starting at `longest_code_expression + 1 space`.
- **Multi-Line Macro Continuation Backslash (`\`) Alignment**:
  - All trailing continuation backslashes (`\`) on `#define` multi-line macros **MUST be aligned to column 130** (standard banner margin).
- **Macro Definition (`#define`) Value & Trailing Comment Alignment**:
  - Within each section, single-line `#define` values **MUST be vertically aligned**.
  - The value column begins at `longest_#define_name + 1 space`.
  - If a macro name or expansion would exceed the maximum line width ($W = 130$ columns), it breaks onto the next line using a continuation backslash (`\`).
  - Trailing comments (`/*!< ... */`) on `#define` lines **MUST be vertically aligned** starting at `longest_code_expression + 1 space`.

```c
enum e_someip_return_code {
    SOMEIP_E_OK                      = 0x00,
    SOMEIP_E_NOT_OK                  = 0x01, /*!< Unspecified error */
    SOMEIP_E_WRONG_INTERFACE_VERSION = 0x08,

    /* Reserved ranges */

    SOMEIP_E_GENERIC_RESERVED_MIN    = 0x10, /*!< 0x10..0x1F: reserved for generic errors */
    SOMEIP_E_GENERIC_RESERVED_MAX    = 0x1F,
};
```

### 6.5 Functions & Multi-Parameter Macros Doxygen Documentation (Compact Documentation)
> [!IMPORTANT]
> **Function & Multi-Param Macro Isolation**:
> Quy tắc tại mục này chỉ áp dụng cho khối tài liệu Doxygen (`/** @... */`) đứng trước **hàm (Functions)** hoặc **macro đa tham số (Multi-Param Function-Like Macros)** có các tag phức tạp như `@details`, `@warning`, `@param`, `@retval`. Không áp dụng định dạng đa cột này vào trong nội bộ phần thân enum, struct hay union.
- Comments should tell a story and document design decisions (*why* a choice was made, not just *what* the code does).
- Place function documentation headers in the `.h` file where public functions are declared, and in the `.c` file for private (`static`) functions (unless the private function name and parameters are completely self-explanatory).
- Keep comment block heights as compact as possible to minimize vertical scrolling:
  - Open documentation directly on the first line with `/** @brief ...` (double-asterisk to activate Doxygen/Clangd LSP tooltip parsing) and close immediately on the last line with `... */`.
  - Avoid standalone `/**` opening and `*/` closing lines.
- Multi-part or extended comments must use explicit Doxygen tags:
  - Use `@brief <summary>` on the first line.
  - Follow with `@details <details>`, `@note <note>`, `@warning <warning>`, `@see <ref>`, `@todo [<author> <date>] <task>`, or `@bug [<author> <date>] <issue>` when applicable. Omit any unnecessary tags.
  - Do not use loose, untagged continuation lines directly beneath `@brief`.
- **Vertical & Horizontal Content Text Column Alignment**:
  1. **Start Column Prefix**: The prefix `/** ` on Line 1 and ` *  ` on subsequent lines maintains a fixed width of 4 characters (`StartCol = 4`).
  2. **Header Tag Alignment**: All header tags (`@brief`, `@details`, `@note`, `@warning`, etc.) are padded to `max(header_tags_length) + 1` space so that all section descriptions begin at the exact same vertical column (Column 12 for standard `@brief`/`@note`, Column 14 when `@details` or `@warning` is present).
  3. **List Items (`-`, `+`, `*`)**: Nested bulleted lists within description text are indented by 2 spaces (` *             - Item`), and their multi-line continuation text is indented by an additional 2 spaces (` *               continued text`) to align directly under the bullet item text.
  4. **Parameter Names Alignment (`@param`)**: Parameter variable names in `@param` statements are vertically aligned to the longest parameter name.
  5. **Parameter Descriptions Alignment**: Parameter description texts begin at the same column (1 space after the longest parameter name).
  6. **Return Values Alignment (`@retval`)**: Return values and their descriptions in `@retval` statements are vertically aligned to the longest return code identifier.

```c
/** @brief   Build the header of the ERROR message that answers a received request
 *  @details Copied from p_rx [PRS_SOMEIP_00190]: Message ID, Request ID [00704], Interface Version.
 *           Set:
 *             - Protocol Version = p_rx->protocol_version, except for SOMEIP_E_WRONG_PROTOCOL_VERSION where it
 *               is SOMEIP_PROTOCOL_VERSION (0x01) (Figure 4.21)
 *             - Message Type = SOMEIP_MT_ERROR (0x81), TP-Flag never set
 *             - Return Code = rc
 *             - Length = SOMEIP_HEADER_PART2_SIZE (8): the payload is NOT copied [PRS_SOMEIP_00190]
 *  @note    p_rx and p_err may point to the same header.
 *  @param   p_rx  Header of the received request
 *  @param   rc    Return Code to send, must not be SOMEIP_E_OK
 *  @param   p_err ERROR header, ready for SOMEIP_HeaderEncode()
 *  @retval  SOMEIP_RES_OK      *p_err is valid
 *  @retval  SOMEIP_RES_E_PARAM p_rx or p_err is NULL, or rc is SOMEIP_E_OK */
```

**DON'T**:

```c
/**
 * @brief Standard Zonal ECU Error Code definitions (ze_err_t)
 * Mapped 1:1 with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals.
 */
```

*or single-asterisk / untagged continuation line (ignored by Doxygen/Clangd LSP):*

```c
/* @brief Standard Zonal ECU Error Code definitions (ze_err_t)
 * Mapped 1:1 with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals. */
```

**DO**:

```c
/** @brief Standard Zonal ECU Error Code definitions (ze_err_t)
 *  @note  Mapped 1:1 with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals. */
```

### 6.6 Function Documentation
- Use compact Doxygen tags with parameter direction qualifiers and `p_` pointer prefixes:

```c
/** @brief Returns constant string representation of a ze_err_t error code
 *  @param err Error code
 *  @return const char* Static string representation of the error name */
const char * ZE_ErrToStr(IN ze_err_t err);
```

---

## 7. Parameter & Permission Macros

Defined in `comp/Common/Config/Macros.h`:
- `RO`: `const` (Read-Only permission).
- `WO`: Write-Only permission.
- `RW`: Read/Write permission.
- `IN`: `const` (Input argument).
- `OUT`: Output argument (must be a pointer `p_...`).
- `INOUT`: Input/Output argument (must be a pointer `p_...`).

---

## 8. Error Code Conventions (`ze_err_t`)

- Error codes are typed as `ze_err_t` (defined in `comp/Common/Error/Error.h`).
- Standard ESP-IDF errors are mapped 1:1 (`ESP_OK`, `ESP_FAIL`, `ESP_ERR_NO_MEM`, ...).
- Subsystem base error codes are allocated in `0x100` increments:
  - `0x1000`: Communication base (`ZE_ERR_BASE_COMM`).
  - `0x1100`: W5500 SPI Ethernet base (`ZE_ERR_BASE_W5500`).
  - `0x1200`: CAN / CAN-FD base (`ZE_ERR_BASE_CAN`).
  - `0x1300`: Storage / NVS base (`ZE_ERR_BASE_STORAGE`).
  - `0x1400`: GPIO / ADC / IO base (`ZE_ERR_BASE_IO`).
