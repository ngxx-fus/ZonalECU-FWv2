# Project Information

T.B.U

---

# Working dir

## 1. Project Directory Structure

```text
ZonalECU-FWv2/
├── .clangd                          # Clangd LSP configuration with Xtensa GCC query-driver
├── .gitignore                       # Git ignore list for ESP-IDF builds
├── CMakeLists.txt                   # Root CMake project script (auto-discovers leaf components)
├── CodingRules.md                   # Source code standards, header layout, and comment rules
├── GEMINI.md                        # Assistant rules, hardware configuration, and architecture
├── sdkconfig.defaults               # Project default configurations (240 MHz, Dual-Core, -O2, No Wi-Fi/BT)
│
├── doc/                             # Doxygen documentation source & assets
│   ├── Doxyfile                     # Doxygen build configuration
│   └── doxygen_html/                # Generated HTML site (gitignored)
│
├── main/                            # Main application entry point
│   ├── CMakeLists.txt               # Main component CMake registration (requires Common)
│   └── ZonalECU-FWv2.c              # Application startup (app_main)
│
└── comp/                            # Modular subsystem components
    ├── .template/                   # Blueprint template for creating new components
    │   ├── CMakeLists.txt           # Production-ready component CMakeLists template
    │   ├── CMakeLists.tutorial.txt  # In-depth CMake guide & syntax breakdown
    │   ├── template.c               # Template source file
    │   └── template.h               # Template public header
    │
    ├── App/                         # Application layer services & middleware
    │   ├── ESP-FreeRTOS/            # FreeRTOS tasks & application lifecycle
    │   ├── ESP-IPC/                 # Inter-process / inter-task communication
    │   ├── SOME-IP/                 # SOME/IP packet serializer & parser
    │   ├── SOME-IP_SD/              # SOME/IP Service Discovery & pairing
    │   └── ZE-Services/             # Zonal ECU core application services
    │
    ├── Comm/                        # Hardware peripheral drivers & communication wrappers
    │   ├── ESP-ClassicCAN/          # Classic CAN driver wrapper
    │   ├── ESP-Flash/               # Flash memory storage wrapper
    │   ├── ESP-GPIO/                # Peripheral GPIO control methods
    │   ├── ESP-I2C/                 # I2C driver wrapper
    │   ├── ESP-Logger/              # UART logging macros & backend
    │   ├── ESP-PSRAM/               # PSRAM allocation & memory management
    │   ├── ESP-SPI/                 # SPI bus driver wrapper
    │   ├── ESP-UART/                # UART communication driver wrapper
    │   └── ETH-W5500/               # W5500 SPI Ethernet middleware
    │
    └── Common/                      # Core cross-cutting definitions
        ├── CMakeLists.txt           # Common component CMake registration
        ├── Atomic/                  # Atomic operations typedefs (Atomic.h)
        ├── Config/                  # Compatibility & attribute macros (Macros.h)
        ├── Error/                   # Standard error codes (Error.h, Error.c)
        ├── __prv_shared__.h         # Private shared type definitions
        └── SharedAPIs.h             # Umbrella header aggregating all Common headers
```

---

## 2. Component Architecture (Leaf Component Discovery)

In standard ESP-IDF projects, having a `CMakeLists.txt` directly inside `components/` causes ESP-IDF to treat all subdirectories as a single monolithic component.

In this project, we use a **Leaf Component Architecture** under `comp/`:
- There is **no** `comp/CMakeLists.txt` at the root of `comp/`.
- Every individual module under `comp/` (at any nesting level, e.g. `comp/Common` or `comp/Comm/W5500`) has its own independent `CMakeLists.txt`.
- The root [CMakeLists.txt](CMakeLists.txt) uses recursive scanning (`file(GLOB_RECURSE ...)`) to automatically register every folder containing a `CMakeLists.txt` under `comp/` into `EXTRA_COMPONENT_DIRS`, automatically ignoring the `.template/` directory.

> [!IMPORTANT]
> **ESP-IDF Wrapper Directory Naming Convention**:
> Any directory or submodule that wraps, adapts, or interfaces directly with an ESP-IDF system driver/API **MUST** begin with the prefix `ESP-` (e.g. `comp/Comm/W5500/ESP-SPI`, `comp/App/ESP-FreeRTOS`, `ESP-UART`).

---

## 3. How to Add a New Component

Follow these simple steps whenever you want to add a new driver, protocol, or application service:

### Step 1: Copy the Template Directory
Duplicate the `comp/.template/` folder to your desired path under `comp/`.
You can create flat or nested components:
```bash
# Example 1: Creating a nested communication driver for W5500
cp -r comp/.template comp/Comm/W5500

# Example 2: Creating a CAN driver
cp -r comp/.template comp/Drivers/CAN
```

### Step 2: Rename and Implement Files
Rename the `.c` and `.h` files to match your component name:
```bash
cd comp/Comm/W5500
mv template.c w5500.c
mv template.h w5500.h
```

Inside your header and source files:
- Update header guards: `#ifndef __COMM_W5500_H__`
- Include necessary common definitions: `#include "Common/SharedAPIs.h"`
- Follow comment and header conventions detailed in [CodingRules.md](CodingRules.md).

### Step 3: Configure Component `CMakeLists.txt`
Open `comp/Comm/W5500/CMakeLists.txt`. The template is pre-configured to automatically scan all `.c` and `.h` files in the component directory and export include paths.

You only need to specify any external dependencies in `REQUIRES`:
```cmake
idf_component_register(
    SRCS ${component_srcs}
    INCLUDE_DIRS ${component_include_dirs}
    REQUIRES Common driver esp_timer   # <-- Add components your module depends on
)
```

### Step 4: Include in Other Components or `main`
Thanks to the template's dual-path include discovery, other modules can include your headers using either style:
```c
/* Module-qualified path (Recommended for clarity) */
#include "Comm/W5500/w5500.h"

/* Direct header inclusion */
#include "w5500.h"
```

Add your component name (the folder name, e.g., `W5500`) to the caller's `REQUIRES` list in its `CMakeLists.txt`. For instance, in `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "ZonalECU-FWv2.c"
    INCLUDE_DIRS "."
    REQUIRES Common W5500
)
```

### Step 5: Build the Project
You **do not** need to edit the root `CMakeLists.txt`. The build system automatically discovers the new component:
```bash
# Sourcing ESP-IDF environment (if not already sourced)
source /home/fus/.fus/esp/export.sh

# Reconfigure and build
idf.py reconfigure
idf.py build
```

---

## 4. Coding & Comment Standards

All developers and contributors must adhere to the standards outlined in:
- [CodingRules.md](CodingRules.md): Detailed conventions for project architecture, hardware specifications, header structure, Doxygen comments (`/** @brief ... */`), section banners ($W = 108$), in-code groupings, macros, and error codes.

---

## 5. Build Environment & Troubleshooting

### 5.1 Standard Build Workflow (Dedicated Terminal)

ESP-IDF projects rely on cross-compilers (`xtensa-esp32-elf-*`), Ninja, CMake, and a dedicated Python virtual environment. Always activate the ESP-IDF environment in your terminal before building:

```bash
# 1. Export ESP-IDF environment variables and toolchains
. /home/fus/.fus/esp/export.sh

# 2. Build the firmware
idf.py build

# 3. Flash and monitor (when hardware is connected)
idf.py -p /dev/ttyUSB0 flash monitor
```

> [!TIP]
> Add an alias to your `~/.bashrc` to quickly initialize the ESP-IDF environment:
> ```bash
> echo 'alias get_idf=". /home/fus/.fus/esp/export.sh"' >> ~/.bashrc
> source ~/.bashrc
> ```
> Then run `get_idf` in any new terminal session before building.

> [!NOTE]
> Running `idf.py build` automatically updates `build/compile_commands.json`. The root [.clangd](.clangd) configuration consumes this compilation database to provide accurate autocompletion, type deduction, and jump-to-definition.

---

### 5.2 Common Issues & Troubleshooting

#### Issue 1: `include could not find requested file: /tools/cmake/project.cmake`
- **Symptom**:
  ```text
  CMake Error at CMakeLists.txt:31 (include):
    include could not find requested file:
      /tools/cmake/project.cmake
  ```
- **Cause**: Line 31 of [CMakeLists.txt](CMakeLists.txt) references `$ENV{IDF_PATH}/tools/cmake/project.cmake`. If CMake is invoked directly or by an IDE extension without an exported `IDF_PATH`, the variable evaluates to empty, resolving incorrectly to root `/tools/cmake/project.cmake`.
- **Solution**:
  - **Terminal**: Run `. /home/fus/.fus/esp/export.sh` before invoking build commands.
  - **VS Code CMake Tools (if used)**: Specify `IDF_PATH` in `.vscode/settings.json` (ensure dictionary `{}` syntax, not array `[]`):
    ```json
    "cmake.environment": {
        "IDF_PATH": "/home/fus/.fus/esp"
    }
    ```

#### Issue 2: `xtensa-esp32-elf-gcc was not found in the PATH`
- **Symptom**:
  ```text
  CMake Error at .../project.cmake (__project):
    The CMAKE_C_COMPILER:
      xtensa-esp32-elf-gcc
    is not a full path and was not found in the PATH.
  ```
- **Cause**: The VS Code **CMake Tools** extension detects `CMakeLists.txt` and automatically attempts to configure the project using a host compiler (e.g. `/usr/bin/gcc`). However, ESP-IDF requires cross-compilation targeting the ESP32 architecture (`xtensa-esp32-elf-gcc`), which is only exposed on `PATH` after sourcing `export.sh`.
- **Solution**:
  If you build in a separate terminal via `idf.py build`, prevent CMake Tools from auto-triggering background configure runs:
  1. Add `"cmake.configureOnOpen": false` to [.vscode/settings.json](.vscode/settings.json).
  2. Click the CMake Kit selector on the VS Code status bar (bottom bar) and select `[Unspecified]`.
  3. Perform all builds via the dedicated terminal using `idf.py build`.

---

## 6. Git Workflow & Commit Guidelines (Recommended)

### 6.1 Branch Naming Convention
```text
ZE-<Feature/Improve/Update/Fix>-<ShortDescription>
```
*Examples*:
- `ZE-Feature-UpdateGitHubAction`
- `ZE-Fix-SPI-Timeout`
- `ZE-Improve-ErrorHandling`

### 6.2 Commit Message Format
```text
<ZE/ZonalECU/ZonalECU-FWv2> | <Timestamp> | <message>
```
*Example*:
```text
ZonalECU-FWv2 | 11/06/26 20:04:03 | Updated static-analysis job check
```

### 6.3 Auto Timestamp Shortcuts

- **Linux / macOS / Git Bash**:
  ```bash
  git commit -m "ZonalECU-FWv2 | $(date +"%D %T") | <message>"
  ```
- **Windows (PowerShell)**:
  ```powershell
  git commit -m "ZonalECU-FWv2 | $(Get-Date -Format 'MM/dd/yy HH:mm:ss') | <message>"
  ```


