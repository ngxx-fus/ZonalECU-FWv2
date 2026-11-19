/********************************************************************************************************************************
 * @file Mutex.c
 * @ingroup Common
 * @brief Source file for [[Mutex]]
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "Mutex.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Batch wait time between retry checks */
#define PRV_MUTEX_BATCH_WAIT_US (100U)

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Begin critical section by disabling hardware interrupts */
ze_err_t ZE_Mutex_CriticalSectionBegin(void) {
   ZE_MUTEX_DISABLE_IRQ();
    return ZE_SUCCESS;
}

/** @brief   End critical section by restoring/enabling hardware interrupts */
ze_err_t ZE_Mutex_CriticalSectionEnd(void) {
   ZE_MUTEX_ENABLE_IRQ();
    return ZE_SUCCESS;
}

/** @brief   Attempt to take mutex lock with timeout and periodic batch delay
 *  @param   p_lock     Pointer to mutex instance
 *  @param   timeout_us Maximum wait time in microseconds (UINT32_MAX for indefinite)
 *  @return  ze_err_t   ZE_SUCCESS if lock taken, ZE_ERR_TIMEOUT if timed out, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_Mutex_LockTake(inout ze_mutex_t rw_ptr p_lock, in uint32_t timeout_us) {
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
           ZE_MUTEX_WATCH_DOG_RESET();
           ZE_MUTEX_YIELD          ();
           ZE_MUTEX_CPU_DELAY_US   (PRV_MUTEX_BATCH_WAIT_US);

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
           ZE_MUTEX_WATCH_DOG_RESET();
           ZE_MUTEX_YIELD          ();

            uint32_t wait_step = (remaining_us < PRV_MUTEX_BATCH_WAIT_US) ? remaining_us : PRV_MUTEX_BATCH_WAIT_US;
           ZE_MUTEX_CPU_DELAY_US(wait_step);

            expected = false;
            if (atomic_compare_exchange_strong(p_lock, &expected, true)) {
                return ZE_SUCCESS;
            }

            if (remaining_us != UINT32_MAX) {
                if (remaining_us <= wait_step) {
                    break;
                }
                remaining_us -= wait_step;
            }
        }

        return ZE_ERR_TIMEOUT;
    #endif
}

/** @brief   Release mutex lock
 *  @param   p_lock Pointer to mutex instance
 *  @return  ze_err_t   ZE_SUCCESS if released, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_Mutex_LockGive(inout ze_mutex_t rw_ptr p_lock) {
    if (IS_NULL(p_lock)) {
        return ZE_ERR_INVALID_ARG;
    }

   atomic_store(p_lock, false);
    return ZE_SUCCESS;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/