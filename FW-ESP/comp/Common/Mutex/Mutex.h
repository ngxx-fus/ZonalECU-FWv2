/********************************************************************************************************************************
 * @file Mutex.h
 * @ingroup ZonalECU_FWv2
 * @brief Header file for [[Mutex]]
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __ZONALECU_FWV2_FW_ESP_COMP_COMMON_MUTEX_MUTEX_H__
#define __ZONALECU_FWV2_FW_ESP_COMP_COMMON_MUTEX_MUTEX_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "../Base/All.h"
#include "../Error/All.h"
#include "../Atomic/All.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef ZE_MUTEX_NO_TIMEOUT_EN
    #define ZE_MUTEX_NO_TIMEOUT_EN     0
#endif

#ifndef ZE_MUTEX_DISABLE_IRQ
    #define ZE_MUTEX_DISABLE_IRQ()      /* Place holder */
#endif

#ifndef ZE_MUTEX_ENABLE_IRQ
    #define ZE_MUTEX_ENABLE_IRQ()       /* Place holder */
#endif

#ifndef ZE_MUTEX_YIELD
    #define ZE_MUTEX_YIELD()            /* Place holder */
#endif

#ifndef ZE_MUTEX_WATCH_DOG_RESET
    #define ZE_MUTEX_WATCH_DOG_RESET()  /* Place holder */
#endif

#ifndef ZE_MUTEX_CPU_DELAY_US
    #define ZE_MUTEX_CPU_DELAY_US(us)   /* Place holder */
#endif

/* Backward compatible aliases */
#ifndef ZE_NO_TIMEOUT_EN
    #define ZE_NO_TIMEOUT_EN           ZE_MUTEX_NO_TIMEOUT_EN
#endif
#ifndef ZE_DISABLE_IRQ
    #define ZE_DISABLE_IRQ()           ZE_MUTEX_DISABLE_IRQ()
#endif
#ifndef ZE_ENABLE_IRQ
    #define ZE_ENABLE_IRQ()            ZE_MUTEX_ENABLE_IRQ()
#endif
#ifndef ZE_YIELD
    #define ZE_YIELD()                 ZE_MUTEX_YIELD()
#endif
#ifndef ZE_WATCH_DOG_RESET
    #define ZE_WATCH_DOG_RESET()       ZE_MUTEX_WATCH_DOG_RESET()
#endif
#ifndef CPU_DELAY_US
    #define CPU_DELAY_US(us)           ZE_MUTEX_CPU_DELAY_US(us)
#endif

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

typedef ze_atomic (bool) ze_mutex_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* SHARED VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Begin critical section by disabling hardware interrupts */
ze_err_t ZE_Mutex_CriticalSectionBegin(void);

/** @brief  End critical section by restoring/enabling hardware interrupts */
ze_err_t ZE_Mutex_CriticalSectionEnd(void);

/** @brief   Attempt to take mutex lock with timeout and periodic batch delay
 *  @param   p_lock     Pointer to mutex instance
 *  @param   timeout_us Maximum wait time in microseconds (UINT32_MAX for indefinite)
 *  @return  ze_err_t   ZE_SUCCESS if lock taken, ZE_ERR_TIMEOUT if timed out, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_Mutex_LockTake(inout ze_mutex_t rw_ptr p_lock, in uint32_t timeout_us);

/** @brief   Release mutex lock
 *  @param   p_lock Pointer to mutex instance
 *  @return  ze_err_t   ZE_SUCCESS if released, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_Mutex_LockGive(inout ze_mutex_t rw_ptr p_lock);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __ZONALECU_FWV2_FW_ESP_COMP_COMMON_MUTEX_MUTEX_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/