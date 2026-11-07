# FW-STM: Assistant Guidelines & Project Rules

This document establishes the interaction rules, architectural boundaries, and hardware specifications for developing the STM32F103C8T6 real-time sub-firmware.

> [!IMPORTANT]
> Detailed source code formatting, naming standards, and header layout rules are maintained in [RULES.md](RULES.md).

---

## 1. Interaction & Workflow Rules
- **Rule Confirmation (Meta-Rule)**: After each user request, if any new rule, project convention, or architectural pattern is established or modified, the assistant must ask the user for confirmation before appending/updating it in `GEMINI.md` or `RULES.md`.
- **Structural Change Confirmation (Mandatory)**: Whenever any structural change is proposed — including file/directory reorganization, moving/renaming/merging files, code architecture modifications, or restructuring documentation — the assistant **MUST** explicitly explain the intended structural changes and ask for user confirmation BEFORE making any edits.
- **Directory README & Context Alignment (Pre-Modification Check)**: Whenever modifying code or configuration in any directory, the assistant **MUST** first read the local `readme.md`, project-level `readme.md`, `GEMINI.md`, and `RULES.md`. Verify whether the local directory's Readme aligns with the intended modification.
- **Brief Pre-Execution Explanation**: The assistant **MUST** provide a brief explanation of what it is about to do and why BEFORE performing any edits or tool executions.
- **Language Convention**: All source code comments, header documentation, and build scripts must be written strictly in **English**.

---

## 2. Hardware Specifications
- **Target Chip**: STM32F103C8T6 (LQFP-48 package).
- **Core**: ARM 32-bit Cortex-M3 CPU running at **72 MHz** (8 MHz HSE external crystal + PLL).
- **Memories**: 64 KB of Flash memory, 20 KB of SRAM.
- **Interfaces**:
  - 1x CAN 2.0B active interface (bxCAN).
  - 2x SPI, 3x USART, 2x I2C.
  - 2x 12-bit ADC (up to 16 channels).
  - Up to 37 fast I/O pins (5V tolerant).
- **Supply Voltage**: 2.0 V to 3.6 V (Standard 3.3V).

---

## 3. Real-Time & Safety Constraints
- **Zero Dynamic Memory Allocation**: No `malloc()`, `calloc()`, or `free()`. All structures must be statically allocated or stack allocated with bounded limits.
- **Deterministic Interrupt Latency**: No loops with unbounded termination conditions inside ISRs.
- **Watchdog Protection**: The Independent Watchdog (IWDG) must be utilized to reset the processor in case of firmware lockup.
- **Code Standards**: Refer to [RULES.md](RULES.md) for full specifications on headers, section banners, and defensive programming.
