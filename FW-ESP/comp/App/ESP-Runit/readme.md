# About

Port of `runit` service supervision to the ESP32 platform.

The initialization process consists of two distinct phases:

1. **Phase 1 — Early Boot (Bare-metal)**: Runs immediately after system reset to handle ZECU errors and diagnostics. This stage prepares and plans execution for the subsequent phase based on accumulated errors.

2. **Phase 2 — RTOS**: The FreeRTOS scheduler is invoked, allowing normal system tasks and services to proceed.