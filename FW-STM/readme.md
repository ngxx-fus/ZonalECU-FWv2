# FW-STM: Real-Time Sub-ECU Firmware

Firmware subsystem targeting the STM32F103C8T6 (ARM Cortex-M3, 72 MHz, 64 KB Flash, 20 KB SRAM). Acts as the real-time sub-controller responsible for deterministic actuator control, sensor acquisition, and Classic CAN bus node communication.

---

## 1. Subsystem Overview

* **Microcontroller**: STM32F103C8T6 ("Blue Pill" / Custom Z-ECU Board).
* **Architecture**: ARM Cortex-M3 32-bit RISC.
* **Clock Frequency**: 72 MHz (HSE 8 MHz + PLL x9).
* **Primary Interfaces**:
  * **CAN 2.0A/B (bxCAN)**: Communicates with Zonal Gateway (ESP32) and vehicle CAN bus.
  * **SPI / UART**: High-speed board-to-board link with ESP32.
  * **GPIO / ADC / PWM**: Low-latency sensor inputs and motor/relay actuator drives.

---

## 2. Directory Structure (Roadmap)

```text
FW-STM/
├── GEMINI.md                        # Assistant rules for STM32 firmware
├── RULES.md                         # Coding rules & MISRA-C guidelines for STM32
├── readme.md                        # This documentation file
│
├── Core/                            # Application Core & Interrupt Handlers (CubeMX generated)
│   ├── Inc/                         # Core header files (main.h, stm32f1xx_it.h)
│   └── Src/                         # Core source files (main.c, stm32f1xx_it.c)
│
├── Drivers/                         # Hardware Abstraction Layer & CMSIS
│   ├── CMSIS/                       # ARM Cortex-M3 core & register definitions
│   └── STM32F1xx_HAL_Driver/        # STMicroelectronics HAL peripheral drivers
│
├── CMakeLists.txt                   # CMake build script for ARM GCC
├── STM32F103C8Tx_FLASH.ld           # GNU Linker Script
└── stm32f103.ioc                    # STM32CubeMX configuration file
```

---

## 3. Toolchain & Build Environment

* **Compiler**: GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`).
* **Build System**: CMake (with arm toolchain file) or GNU Make.
* **Flashing / Debugging**: OpenOCD / ST-Link V2 / J-Link via SWD interface.

```bash
# Compilation (when configured)
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../arm-toolchain.cmake ..
make -j$(nproc)
```

---

## 4. Coding Standards

* All code in `FW-STM` must comply with [RULES.md](file:///home/fus/Documents/ZonalECU-FWv2/FW-STM/RULES.md).
* Strict type sizing using `<stdint.h>` (`uint8_t`, `uint16_t`, `uint32_t`).
* Deterministic timing: No unbounded blocking loops in ISRs; watchdog timer (IWDG) must be refreshed periodically.
