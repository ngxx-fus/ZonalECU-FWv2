# ESP-Mutex Component

## Overview
`ESP-Mutex` is the ESP32 hardware and RTOS adaptation layer for the multi-target `Common/Mutex` subsystem.
It wraps generic atomic spinlocks (`ze_mutex_t`) with ESP32-specific hardware and FreeRTOS capabilities:
- **Fast microsecond spinlock**: Backed by `esp_rom_delay_us` for low-latency (< 1ms) locking.
- **Watchdog-safe long wait**: Automatically switches to FreeRTOS tick yields (`vTaskDelay(1)`) and feeds `esp_task_wdt_reset()` when waiting $\ge 1\text{ ms}$, preventing Task Watchdog Timer (TWDT) starvations on ESP32 dual cores.
- **Core interrupt management**: Implements `portDISABLE_INTERRUPTS()` and `portENABLE_INTERRUPTS()` for critical sections.

## APIs
- `ZE_ESP_Mutex_LockTake(p_lock, timeout_us)`: Attempts to acquire lock with hybrid wait.
- `ZE_ESP_Mutex_LockGive(p_lock)`: Releases lock atomically.
