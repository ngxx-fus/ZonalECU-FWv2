/********************************************************************************************************************************
 * @file ESP-Mutex.c
 * @ingroup FW_ESP
 * @brief Source file for [[ESP-Mutex]] wrapper
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "ESP-Mutex.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Threshold in microseconds above which FreeRTOS tick delay is used to prevent watchdog starvation */
#define PRV_RTOS_YIELD_THRESHOLD_US (1000U)
#define PRV_MUTEX_BATCH_WAIT_US     (100U)

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

static portMUX_TYPE s_critical_mux = portMUX_INITIALIZER_UNLOCKED;

/* PUBLIC VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Begin critical section with FreeRTOS SMP portMUX spinlock and interrupt masking */
ze_err_t ZE_ESP_Mutex_CriticalSectionBegin(void) {
   taskENTER_CRITICAL(&s_critical_mux);
    return ZE_SUCCESS;
}

/** @brief  End critical section restoring FreeRTOS SMP portMUX spinlock and interrupts */
ze_err_t ZE_ESP_Mutex_CriticalSectionEnd(void) {
   taskEXIT_CRITICAL(&s_critical_mux);
    return ZE_SUCCESS;
}

/** @brief   Attempt to take mutex lock with ESP32 hybrid delay to prevent watchdog starvation
 *  @param   p_lock     Pointer to mutex instance
 *  @param   timeout_us Maximum wait time in microseconds (UINT32_MAX for indefinite)
 *  @return  ze_err_t   ZE_SUCCESS if lock taken, ZE_ERR_TIMEOUT if timed out, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_ESP_Mutex_LockTake(inout ze_esp_mutex_t rw_ptr p_lock, in uint32_t timeout_us) {
    if (IS_NULL(p_lock)) {
        return ZE_ERR_INVALID_ARG;
    }

    /* Fast path: non-blocking immediate acquisition attempt */
    bool expected = false;
    if (atomic_compare_exchange_strong(p_lock, &expected, true)) {
        return ZE_SUCCESS;
    }

    #if (ZE_MUTEX_NO_TIMEOUT_EN == 1)
       ZE_UNUSED(timeout_us);
        while (true) {
            (void)esp_task_wdt_reset();
           vTaskDelay(pdMS_TO_TICKS(1));

            expected = false;
            if (atomic_compare_exchange_strong(p_lock, &expected, true)) {
                return ZE_SUCCESS;
            }
        }
    #else
        if (0U == timeout_us) {
            return ZE_ERR_TIMEOUT;
        }

        uint32_t remaining_us = timeout_us;

        while (true) {
            (void)esp_task_wdt_reset();

            /* If waiting for more than 1ms, yield via FreeRTOS tick delay so IDLE task can feed watchdog */
            if (remaining_us >= PRV_RTOS_YIELD_THRESHOLD_US) {
               vTaskDelay(pdMS_TO_TICKS(1));
                if (remaining_us != UINT32_MAX) {
                    if (remaining_us <= PRV_RTOS_YIELD_THRESHOLD_US) {
                        break;
                    }
                    remaining_us -= PRV_RTOS_YIELD_THRESHOLD_US;
                }
            } else {
               taskYIELD();
                uint32_t wait_step = (remaining_us < PRV_MUTEX_BATCH_WAIT_US) ? remaining_us : PRV_MUTEX_BATCH_WAIT_US;
               esp_rom_delay_us(wait_step);

                if (remaining_us != UINT32_MAX) {
                    if (remaining_us <= wait_step) {
                        break;
                    }
                    remaining_us -= wait_step;
                }
            }

            expected = false;
            if (atomic_compare_exchange_strong(p_lock, &expected, true)) {
                return ZE_SUCCESS;
            }
        }

        return ZE_ERR_TIMEOUT;
    #endif
}

/** @brief   Release mutex lock
 *  @param   p_lock Pointer to mutex instance
 *  @return  ze_err_t   ZE_SUCCESS if released, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_ESP_Mutex_LockGive(inout ze_esp_mutex_t rw_ptr p_lock) {
    if (IS_NULL(p_lock)) {
        return ZE_ERR_INVALID_ARG;
    }

   atomic_store(p_lock, false);
    return ZE_SUCCESS;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/