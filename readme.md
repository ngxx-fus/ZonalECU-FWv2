# Zonal ECU Firmware v2 (ZonalECU-FWv2)

Enterprise-grade firmware architecture for Automotive Zonal Electronic Control Units (Z-ECU). This repository hosts a multi-target firmware ecosystem designed for distributed real-time vehicle networking, service-oriented communication (SOME/IP), and deterministic IO actuator control.

---

## 1. Multi-Target System Architecture

```text
       +-------------------------------------------------------+
       |                  Ethernet / SOME/IP                   |
       +---------------------------+---------------------------+
                                   |
                                   v
             +-------------------------------------------+
             |                  FW-ESP                   |
             |       (ESP32 Gateway & Core Services)     |
             |   - SOME/IP & Service Discovery (SD)      |
             |   - FreeRTOS Tasks & Lifecycle Management |
             |   - Ethernet (W5500 SPI) & Network Stacks |
             +---------------------+---------------------+
                                   |
                                   | Classic CAN / SPI / UART
                                   v
             +-------------------------------------------+
             |                  FW-STM                   |
             |       (STM32F103C8T6 Real-Time Sub-ECU)  |
             |   - Deterministic Low-Latency Actuation   |
             |   - Sensor Acquisition & Power Control    |
             |   - Classic CAN Bus Node                  |
             +-------------------------------------------+
```

---

## 2. Repository Directory Structure

```text
ZonalECU-FWv2/
├── .clangd                          # Project-wide Clangd LSP configuration
├── .gitignore                       # Repository-wide ignore rules
├── readme.md                        # Master project documentation
│
├── .github/                         # Continuous Integration & Delivery
│   ├── workflows/
│   │   └── ci.yml                   # 6-stage CI/CD pipeline
│   └── scripts/
│       ├── build_docs.sh            # Doxygen multi-target documentation builder
│       ├── build_firmware.sh        # ESP-IDF firmware compilation script
│       ├── check_formatting.sh      # Automated code formatting verifier (CI gate)
│       ├── check_git_conventions.sh # Commit message & convention validator
│       ├── clangd_check.py          # Python LSP JSON-RPC static analyzer
│       ├── format_rules.py          # Auto-code reformatter (Renesas FSP, 110-col banners)
│       └── static_analysis.sh       # Clangd static analysis orchestrator
│
├── FW-Common/                       # Shared Cross-Target Resources
│   ├── doc/                         # Master Doxygen documentation configuration
│   │   ├── Doxyfile                 # Doxygen config (aggregates FW-ESP & FW-STM)
│   │   ├── DoxygenLayout.xml        # Custom navigation layout
│   │   └── doxygen_assets/          # Logo, custom styles, and navtree scripts
│   ├── drawio/                      # System architecture diagrams
│   │   └── SystemView.drawio        # Z-ECU electrical & network block diagrams
│   └── readme.md                    # Common resources overview
│
├── FW-ESP/                          # ESP32 Firmware Subsystem
│   ├── CMakeLists.txt               # ESP-IDF project build script
│   ├── CodingRules.md               # C coding standards & header layout rules
│   ├── GEMINI.md                    # AI agent & developer guidelines for ESP32
│   ├── readme.md                    # ESP32 subsystem documentation & build guide
│   ├── sdkconfig.defaults           # Default configuration (240MHz, Dual-Core, No Wi-Fi/BT)
│   ├── main/                        # Application entry point (app_main)
│   ├── comp/                        # 3-Tier component architecture
│   │   ├── App/                     # High-level services (SOME-IP, FreeRTOS, IPC, ZE-Services)
│   │   ├── Comm/                    # Hardware drivers & wrappers (ETH-W5500, UART, SPI, CAN...)
│   │   └── Common/                  # Shared macros, atomic types, and error codes
│   └── data/                        # Static assets & binary partition files
│
└── FW-STM/                          # STM32F103C8T6 Firmware Subsystem
    ├── GEMINI.md                    # AI agent & developer guidelines for STM32
    ├── RULES.md                     # Coding rules for STM32 firmware
    ├── readme.md                    # STM32 subsystem documentation & roadmap
    └── (Core/, Drivers/ ...)        # CubeMX / CMSIS / HAL drivers (To be populated)
```

---

## 3. Subsystem Overview

| Subsystem | Target Processor | Primary Role | Toolchain |
| :--- | :--- | :--- | :--- |
| [**FW-ESP**](file:///home/fus/Documents/ZonalECU-FWv2/FW-ESP/readme.md) | ESP32-WROOM-32 (Xtensa Dual-Core 240MHz) | Gateway, SOME/IP Protocol, Ethernet W5500, Core Services | ESP-IDF v6.1 (`xtensa-esp32-elf-gcc`) |
| [**FW-STM**](file:///home/fus/Documents/ZonalECU-FWv2/FW-STM/readme.md) | STM32F103C8T6 (ARM Cortex-M3 72MHz) | Real-time IO, Actuator control, Classic CAN bus node | ARM GNU Toolchain (`arm-none-eabi-gcc`) |
| [**FW-Common**](file:///home/fus/Documents/ZonalECU-FWv2/FW-Common/readme.md) | Multi-target | Shared diagrams, shared headers, master Doxygen documentation | Doxygen, Graphviz, Draw.io |

---

## 4. Continuous Integration Pipeline (CI/CD)

The GitHub Actions workflow runs on every push and pull request across all master branches:

| Stage | Job Name | Responsibility |
| :--- | :--- | :--- |
| **Job 0** | **Conventions & Formatting** | Validates commit messages and verifies code formatting rules (fails on unformatted code). |
| **Job 1** | **Clangd Static Analysis** | Performs deep AST & Clang-Tidy analysis across `FW-Common`, `FW-ESP`, `FW-STM`. |
| **Job 2** | **FW-STM Build** | Placeholder for STM32 CMake / Makefile build using `arm-none-eabi-gcc`. |
| **Job 3** | **FW-ESP Build** | Compiles ESP-IDF firmware and publishes binary flash artifacts. |
| **Job 4** | **Project Doxygen** | Aggregates code from `FW-ESP` and `FW-STM` into interactive HTML documentation. |
| **Job 5** | **Deploy to Pages** | Publishes documentation to GitHub Pages on merges to master branches. |

---

## 5. Git Commit Conventions

All commits (excluding automatic pull request merge commits) MUST strictly follow the format:

```text
<REPO> | <DATETIME> | <TAG> | <COMMENT>
```

### Components
* `<REPO>`: Project identifier (`ZonalECU-FWv2` or `ZE`).
* `<DATETIME>`: Timestamp formatted as `MM/DD/YY HH:MM:SS`.
* `<TAG>`: Classification tag:
  * `FEAT`: New feature or hardware peripheral implementation.
  * `FIX`: Bug fix, error code resolution, or assertion fix.
  * `ARCH`: Structural reorganization, CMake modifications, or directory updates.
  * `DOCS`: Documentation additions or updates.
  * `REVERT`: Revert of a previous commit.
  * `MISC`: Maintenance, code style, or CI workflow tweaks.
* `<COMMENT>`: Clear, concise explanation in English of the modification.

### Example
```bash
git commit -m "ZonalECU-FWv2 | $(date +"%D %T") | ARCH | Restructured into multi-target architecture FW-ESP and FW-STM"
```
