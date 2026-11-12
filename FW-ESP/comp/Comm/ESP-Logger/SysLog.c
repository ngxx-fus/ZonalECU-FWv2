/********************************************************************************************************************************
 * @file SysLog.c
 * @ingroup ESP-Logger
 * @brief Source file for SysLog
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "SysLog.h"
#include "Common/All.h"
#include "Common/Atomic/Atomic.h"
#include "Common/Bitwise/Bitwise.h"
#include "Common/Error/Error.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define SYS_LOG_TIMEOUT (0x2000U)
#ifndef CPU_YIELD
#define CPU_YIELD() /* Place holder */
#endif
#ifndef CPU_RESET_WDT
#define CPU_WDT_RESET() /* Place holder */
#endif

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

typedef enum e_sys_log_flag {
    SYS_LOG_NOT_FLAGGED     = 0x0, /* None of flag is set */
    SYS_LOG_RESOURCE_LOCKED = 0x1, /* CORE_LOG is locked (in-use) */
    SYS_LOG_RESOURCE_INIT   = 0x2, /* Resource has been initialized */
} sys_log_flag_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

static ZE_ATOMIC_NEW(uint32_t, prv_sys_log_flag);

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

ze_err_t prv_sys_log_resource_take() {
    uint32_t prv_last_val;
    volatile uint32_t timeout = SYS_LOG_TIMEOUT;
    ze_atomic_uint32_t *p_flag = &prv_sys_log_flag;
    while (timeout--) {
        prv_last_val = ZE_ATOMIC_OR(p_flag, UINT32_BIT(SYS_LOG_RESOURCE_LOCKED));
        if (UINT32_IS_SET(prv_last_val, SYS_LOG_RESOURCE_LOCKED)) {

            /* Resource already locked by another threads! */
            CPU_YIELD();
            CPU_WDT_RESET();
            continue;
        }

        /* Resource locked by this turn! */
        break;
    }

    if (timeout == UINT32_MAX) {
        return ZE_ERR_CANT_TAKE_LOCK;
    }

    return ZE_SUCCESS;
}

ze_err_t prv_sys_log_resource_give() {
    ze_atomic_uint32_t *p_flag = &prv_sys_log_flag;
    uint32_t prv_last_val = ZE_ATOMIC_AND(p_flag, BW_INV(UINT32_BIT(SYS_LOG_RESOURCE_LOCKED)));
    if (UINT32_IS_CLR(prv_last_val, SYS_LOG_RESOURCE_LOCKED)) {
        return ZE_ERR_RESOURCE_FREE;
    }
    return ZE_SUCCESS;
}

/* PUBLIC VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

ze_err_t ZE_SysLog_Init() {
    uint32_t prv_flag = ZE_ATOMIC_GET(&prv_sys_log_flag);
    if (UINT32_IS_SET(prv_flag, SYS_LOG_RESOURCE_LOCKED)) {
        return ZE_ERR_RESOURCE_LOCKED;
    }
    if (UINT32_IS_SET(prv_flag, SYS_LOG_RESOURCE_INIT)) {
        return ZE_ERR_ALREADY_INIT;
    }
    /* Place the code uart here! */
    return ZE_SUCCESS;
}

ze_err_t ZE_SysLog_Printf(const char *p_fmt, ...) {
    /* Place the code uart here! */

    return ZE_SUCCESS;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/