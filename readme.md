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
│       ├── build_firmware_esp.sh    # ESP-IDF firmware compilation script
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
│   ├── scripts/                     # Pre-commit & working tree update tools
│   │   ├── working_tree_update.ps1  # Windows PowerShell local pre-commit runner (Job 0 & 1)
│   │   └── working_tree_update.sh   # Linux / WSL / Bash local pre-commit runner (Job 0 & 1)
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

### 4.1 Prerequisites Installation

Install the required host toolchain and verification utilities for your operating system:

#### A. Void Linux (`xbps-install`)
```bash
# Synchronize package repositories
sudo xbps-install -S

# Core build system & Python
sudo xbps-install -y base-devel cmake ninja git python3 python3-pip

# Job 0 & Job 1: Clang-Format & Clangd Language Server (provided by clang-tools-extra / llvm)
sudo xbps-install -y clang-tools-extra llvm

# Job 4: Doxygen & Graphviz (Documentation generation)
sudo xbps-install -y doxygen graphviz

# Job 2: ARM GNU Toolchain (for STM32 sub-firmware)
sudo xbps-install -y cross-arm-none-eabi-gcc cross-arm-none-eabi-newlib
```

#### B. Ubuntu / Debian (`apt-get`)
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git python3 python3-pip
sudo apt-get install -y clangd clang-format
sudo apt-get install -y doxygen graphviz
sudo apt-get install -y gcc-arm-none-eabi
```

#### C. Windows (WSL2 & Native PowerShell)

* **Option 1: Windows Subsystem for Linux (WSL2 — Recommended)**
  ```powershell
  # In PowerShell (Admin):
  wsl --install
  ```
  Once inside the WSL2 Ubuntu terminal, run the **Ubuntu / Debian** commands above.

* **Option 2: Native Windows (via `winget`)**
  Run in PowerShell:
  ```powershell
  # Core tools & Compilers
  winget install --id Git.Git -e
  winget install --id Kitware.CMake -e
  winget install --id Ninja-build.Ninja -e
  winget install --id Python.Python.3.11 -e
  winget install --id LLVM.LLVM -e                      # Provides clangd & clang-format
  winget install --id DimitriVanHeesch.Doxygen -e       # Doxygen
  winget install --id Graphviz.Graphviz -e               # Graphviz
  winget install --id Arm.GnuArmEmbeddedToolchain -e    # ARM GCC
  ```

> [!NOTE]
> **Job 3 (ESP-IDF v6.1)**: Requires the official Espressif IoT Development Framework.
> - **Linux / WSL2**: Clone and run the installer:
>   ```bash
>   git clone -b release/v6.1 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf
>   cd ~/esp/esp-idf && ./install.sh esp32
>   . ~/esp/esp-idf/export.sh
>   ```
> - **Windows Native**: Use the official [ESP-IDF Windows Offline Installer](https://dl.espressif.com/dl/esp-idf/).

---

### 4.2 Local Pre-Commit Runner & Auto-Formatting (Recommended)

Before creating a commit or opening a Pull Request, run the local pre-commit script. It ports **Job 0 (Formatting & Conventions)** and **Job 1 (Clangd Static Analysis & Strict Compiler Checks)** into a single local command that auto-formats your working tree, verifies Git conventions, and checks for static errors.

#### Windows (PowerShell)
```powershell
# Standard run: auto-formats code, verifies branch/commit format, and runs static analysis:
pwsh -File .\FW-Common\scripts\working_tree_update.ps1

# Strict check-only mode (fails with exit 1 if any unformatted code is detected):
pwsh -File .\FW-Common\scripts\working_tree_update.ps1 -CheckOnly

# Skip Git branch and commit conventions check:
pwsh -File .\FW-Common\scripts\working_tree_update.ps1 -SkipGitConventions

# Analyze only specific files:
pwsh -File .\FW-Common\scripts\working_tree_update.ps1 -Files "FW-ESP\main\ZonalECU-FWv2.c"
```

#### Linux / WSL / macOS (Bash)
```bash
# Standard run:
./FW-Common/scripts/working_tree_update.sh

# Strict check-only mode:
./FW-Common/scripts/working_tree_update.sh --check-only

# Skip Git conventions check:
./FW-Common/scripts/working_tree_update.sh --skip-git-conventions
```

> [!TIP]
> The script auto-discovers Espressif toolchains (`esp-clang`, `esp-clangd`, `xtensa-esp-elf`) and system Python/LLVM paths automatically, eliminating the need to manually set PATH on Windows.

---

### 4.3 Running Individual CI Scripts Locally

Developers should run the verification scripts before pushing code to avoid failing PR checks:

#### Job 0: Code Formatting & Git Conventions Check
```bash
# Auto-format all target C and H files:
python3 .github/scripts/format_rules.py FW-Common FW-ESP FW-STM

# Verify formatting compliance (fails with diff if unformatted):
./.github/scripts/check_formatting.sh

# Verify Git branch & commit message convention:
./.github/scripts/check_git_conventions.sh
```

#### Job 1: Clangd Static Analysis (Deep AST & Semantic Checks)
```bash
# Runs clangd LSP against FW-Common, FW-ESP, and FW-STM
./.github/scripts/static_analysis.sh
```

#### Job 3: Build ESP32 Firmware (ESP-IDF)
```bash
# Compiles binaries to FW-ESP/build/
./.github/scripts/build_firmware_esp.sh
```

#### Job 4: Generate Multi-Target Doxygen Documentation
```bash
# Generates interactive HTML documentation in FW-Common/doc/doxygen_html/
./.github/scripts/build_docs.sh
```

---

### 4.4 Pre-Push All-in-One Check

Run all local CI validations in a single command before submitting a Pull Request:

```bash
./.github/scripts/check_formatting.sh && \
./.github/scripts/check_git_conventions.sh && \
./.github/scripts/static_analysis.sh && \
./.github/scripts/build_firmware_esp.sh && \
./.github/scripts/build_docs.sh
```

---

## 5. Git Commit Conventions

All commits (excluding automatic pull request merge commits) MUST strictly follow the format:

```text
<REPO> | <DATETIME> | <TAG> | <COMMENT>
```

### Components
* `<REPO>`: Project identifier (`ZonalECU-FWv2` or `ZE`).
* `<DATETIME>`: Timestamp formatted as `MM/DD/YY HH:MM:SS` (or `MM-DD-YYYY HH:MM:SS`).
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
