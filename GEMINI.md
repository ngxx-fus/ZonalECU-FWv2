# ZonalECU-FWv2 Master Guidelines & Architecture

This document persists repository-level rules, multi-target architecture definitions, and workflow guidelines for AI coding assistants and developers.

> [!IMPORTANT]
> - Detailed ESP32 rules and coding standards are maintained in [FW-ESP/GEMINI.md](FW-ESP/GEMINI.md) and [FW-ESP/CodingRules.md](FW-ESP/CodingRules.md).
> - Detailed STM32 rules and coding standards are maintained in [FW-STM/GEMINI.md](FW-STM/GEMINI.md) and [FW-STM/RULES.md](FW-STM/RULES.md).

---

## 1. Interaction & Workflow Rules
- **Rule Confirmation (Meta-Rule)**: After each user request, if any new rule, project convention, or architectural pattern is established or modified, the assistant must ask the user for confirmation before appending/updating it in `GEMINI.md` or `CodingRules.md`.
- **Structural Change Confirmation (Mandatory)**: Whenever any structural change is proposed — including file/directory reorganization, moving/renaming/merging files, code architecture modifications, or restructuring documentation — the assistant **MUST** explicitly explain the intended structural changes and ask for user confirmation BEFORE making any edits.
- **Directory README & Context Alignment (Pre-Modification Check)**: Whenever modifying code or configuration in any directory, the assistant **MUST** first read the local `readme.md`, project-level `readme.md`, `GEMINI.md`, and local rules. Verify whether the local directory's Readme aligns with the intended modification.
- **Brief Pre-Execution Explanation**: The assistant **MUST** provide a brief explanation of what it is about to do and why BEFORE performing any edits or tool executions.
- **Troubleshooting & Engineering Lessons Log (`NoteWhileFixing.md` — Mandatory)**: Whenever any bug, CI failure, tool conflict, or technical limitation is diagnosed and resolved, the assistant **MUST** document the issue symptom, root cause analysis, fix applied, and preventive lesson in [`NoteWhileFixing.md`](NoteWhileFixing.md) at the repository root.
- **Language Convention**: All source code comments, header file documentation, function descriptions, and build scripts must be written strictly in **English**.

---

## 2. Multi-Target Ecosystem Overview

| Directory | Target Hardware | Purpose | Build System |
| :--- | :--- | :--- | :--- |
| [**FW-ESP/**](FW-ESP/readme.md) | ESP32-WROOM-32 (Xtensa Dual-Core 240MHz) | Core Gateway, Ethernet W5500, SOME/IP Protocol, Lifecycle | ESP-IDF v6.1 (`xtensa-esp32-elf-gcc`) |
| [**FW-STM/**](FW-STM/readme.md) | STM32F103C8T6 (ARM Cortex-M3 72MHz) | Real-time Sub-ECU, Sensor Acquisition, Actuator Control, CAN Node | ARM GCC (`arm-none-eabi-gcc`) |
| [**FW-Common/**](FW-Common/readme.md) | Multi-target | Shared diagrams (`drawio/`), Cross-target docs & Doxygen (`doc/`) | Doxygen / Graphviz |

---

## 3. Toolchain & CI/CD Pipeline
- **Clangd Static Analysis**: `.clangd` is maintained at project root pointing to `FW-ESP/build/compile_commands.json` to enable full semantic AST analysis across all subsystems.
- **GitHub Actions (`.github/workflows/ci.yml`)**:
  - `Job 0`: Git Conventions check.
  - `Job 1`: Clangd static analysis across `FW-Common`, `FW-ESP`, and `FW-STM`.
  - `Job 2`: `FW-STM` build placeholder.
  - `Job 3`: `FW-ESP` build using ESP-IDF v6.1.
  - `Job 4`: Project Doxygen documentation (outputs to `FW-Common/doc/doxygen_html`).
  - `Job 5`: Deploy documentation to GitHub Pages.
