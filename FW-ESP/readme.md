# FW-ESP: Zonal ECU Gateway & Core Firmware

Firmware subsystem targeting the ESP32 (Xtensa Dual-Core 240MHz). Acts as the primary network gateway, host for AUTOSAR SOME/IP communication, Ethernet W5500 interface, and core application supervisor.

---

## 1. Directory Structure

```text
FW-ESP/
├── .clangd                          # Local Clangd LSP configuration
├── .gitignore                       # ESP-IDF specific build ignores
├── CMakeLists.txt                   # Component auto-discovery build script
├── CodingRules.md                   # Source code standards & include conventions
├── GEMINI.md                        # Assistant rules for FW-ESP
├── readme.md                        # This documentation file
├── sdkconfig.defaults               # Default build settings (240MHz, -O2, Dual-Core)
│
├── main/                            # Application entry point
│   ├── CMakeLists.txt               # Main component registration
│   └── ZonalECU-FWv2.c              # Application startup (app_main)
│
├── data/                            # Static storage and partition assets
│
└── comp/                            # 3-Tier Modular Component Tree
    ├── .template/                   # Blueprint template for creating new components
    │
    ├── App/                         # Application Layer Services & Protocols
    │   ├── ESP-FreeRTOS/            # FreeRTOS task abstractions & lifecycle
    │   ├── ESP-IPC/                 # Inter-process / queue communication
    │   ├── SOME-IP/                 # AUTOSAR SOME/IP serializer & parser
    │   ├── SOME-IP_SD/              # SOME/IP Service Discovery
    │   └── ZE-Services/             # Zonal ECU vehicle application services
    │
    ├── Comm/                        # Communication & Hardware Peripheral Wrappers
    │   ├── ESP-ClassicCAN/          # Classic CAN driver wrapper
    │   ├── ESP-Flash/               # Flash memory storage wrapper
    │   ├── ESP-GPIO/                # Peripheral GPIO control methods
    │   ├── ESP-I2C/                 # I2C driver wrapper
    │   ├── ESP-Logger/              # UART logging macros & backend
    │   ├── ESP-PSRAM/               # PSRAM allocation & memory management
    │   ├── ESP-SPI/                 # SPI bus driver wrapper
    │   ├── ESP-UART/                # UART communication driver wrapper
    │   └── ETH-W5500/               # Ethernet W5500 driver
    │
    └── Common/                      # Shared Types, Macros, & Core Definitions
        ├── Atomic/                  # Atomic types & operations
        ├── Config/                  # Global system configuration & macros
        ├── Error/                   # Error codes & result types
        ├── SharedAPIs.h             # Common API declarations
        └── __prv_shared__.h         # Private shared definitions
```

---

## 2. Naming & Include Conventions

1. **ESP Wrapper Prefix**: All components wrapping ESP-IDF drivers or hardware APIs must use the `ESP-` prefix (e.g. `ESP-FreeRTOS`, `ESP-UART`, `ESP-SPI`).
2. **Header Namespace & Anti-Collision Rule**:
   * For generic/shared headers (`Shared_APIs.h`, `Config.h`), **always** use layer or component namespace qualification:
     ```c
     #include "App/ESP-FreeRTOS/Shared_APIs.h"
     /* or */
     #include "ESP-FreeRTOS/Shared_APIs.h"
     ```
   * **Prohibited**: Bare `#include "Shared_APIs.h"` (prevents cross-component header collisions).
   * Globally unique headers may be included bare:
     ```c
     #include "Error.h"
     #include "Macros.h"
     #include "Atomic.h"
     ```
3. **Permission & Pointer Annotation Macros (`Macros.h`)**:
   * Permission and pointer annotation macros are available in 4 equivalent style sets:
     1. **Uppercase**: `RO`, `WO`, `RW`, `IN`, `OUT`, `INOUT`, `PTR_RO`, `PTR_WO`, `PTR_RW`
     2. **Lowercase**: `ro`, `wo`, `rw`, `in`, `out`, `inout`, `ptr_ro`, `ptr_wo`, `ptr_rw`
     3. **Leading `__` Uppercase**: `__RO`, `__WO`, `__RW`, `__IN`, `__OUT`, `__INOUT`, `__PTR_RO`, `__PTR_WO`, `__PTR_RW`
     4. **Leading `__` Lowercase**: `__ro`, `__wo`, `__rw`, `__in`, `__out`, `__inout`, `__ptr_ro`, `__ptr_wo`, `__ptr_rw`
   * **Single-Style-Per-File Consistency Rule**: Developers may choose any of the 4 styles freely based on personal/subsystem preference, but **MUST use strictly ONE single style per source/header file** (never mix different casing or prefix styles within the same file).


---

## 3. How to Build & Flash

### Prerequisites
* ESP-IDF v6.1 installed and environment exported.

### Building Firmware
From repository root or inside `FW-ESP/`:
```bash
cd FW-ESP
idf.py set-target esp32
idf.py build
```

### Flashing & Monitoring
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```
