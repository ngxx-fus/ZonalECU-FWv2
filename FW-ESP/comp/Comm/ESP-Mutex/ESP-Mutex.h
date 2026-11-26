/********************************************************************************************************************************
 * @file ESP-Mutex.h
 * @ingroup FW_ESP
 * @brief Header file for [[ESP-Mutex]] wrapper
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __FW_ESP_COMP_COMM_ESP_MUTEX_ESP_MUTEX_H__
#define __FW_ESP_COMP_COMM_ESP_MUTEX_ESP_MUTEX_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "esp_rom_sys.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Common/Base/All.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef ZE_MUTEX_NO_TIMEOUT_EN
    #define ZE_MUTEX_NO_TIMEOUT_EN     0
#endif

#ifndef ZE_MUTEX_CPU_DELAY_US
    #define ZE_MUTEX_CPU_DELAY_US(us)  esp_rom_delay_us(us)
#endif

#ifndef ZE_MUTEX_YIELD
    #define ZE_MUTEX_YIELD()           taskYIELD()
#endif

#ifndef ZE_MUTEX_DISABLE_IRQ
    #define ZE_MUTEX_DISABLE_IRQ()     portDISABLE_INTERRUPTS()
#endif

#ifndef ZE_MUTEX_ENABLE_IRQ
    #define ZE_MUTEX_ENABLE_IRQ()      portENABLE_INTERRUPTS()
#endif

#ifndef ZE_MUTEX_WATCH_DOG_RESET
    #define ZE_MUTEX_WATCH_DOG_RESET() esp_task_wdt_reset()
#endif

/* Aliases */
#ifndef ZE_NO_TIMEOUT_EN
    #define ZE_NO_TIMEOUT_EN           ZE_MUTEX_NO_TIMEOUT_EN
#endif
#ifndef CPU_DELAY_US
    #define CPU_DELAY_US(us)           ZE_MUTEX_CPU_DELAY_US(us)
#endif
#ifndef ZE_YIELD
    #define ZE_YIELD()                 ZE_MUTEX_YIELD()
#endif
#ifndef ZE_DISABLE_IRQ
    #define ZE_DISABLE_IRQ()           ZE_MUTEX_DISABLE_IRQ()
#endif
#ifndef ZE_ENABLE_IRQ
    #define ZE_ENABLE_IRQ()            ZE_MUTEX_ENABLE_IRQ()
#endif
#ifndef ZE_WATCH_DOG_RESET
    #define ZE_WATCH_DOG_RESET()       ZE_MUTEX_WATCH_DOG_RESET()
#endif

#include "Common/Mutex/All.h"

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

typedef ze_mutex_t ze_esp_mutex_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* SHARED VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Begin critical section with FreeRTOS SMP portMUX spinlock and interrupt masking */
ze_err_t ZE_ESP_Mutex_CriticalSectionBegin(void);

/** @brief  End critical section restoring FreeRTOS SMP portMUX spinlock and interrupts */
ze_err_t ZE_ESP_Mutex_CriticalSectionEnd(void);

/** @brief   Attempt to take mutex lock with ESP32 hybrid delay to prevent watchdog starvation
 *  @param   p_lock     Pointer to mutex instance
 *  @param   timeout_us Maximum wait time in microseconds (UINT32_MAX for indefinite)
 *  @return  ze_err_t   ZE_SUCCESS if lock taken, ZE_ERR_TIMEOUT if timed out, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_ESP_Mutex_LockTake(inout ze_esp_mutex_t rw_ptr p_lock, in uint32_t timeout_us);

/** @brief   Release mutex lock
 *  @param   p_lock Pointer to mutex instance
 *  @return  ze_err_t   ZE_SUCCESS if released, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_ESP_Mutex_LockGive(inout ze_esp_mutex_t rw_ptr p_lock);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __FW_ESP_COMP_COMM_ESP_MUTEX_ESP_MUTEX_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/