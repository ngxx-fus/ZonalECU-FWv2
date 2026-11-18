/********************************************************************************************************************************
 * @file SysLog.c
 * @ingroup ESP-Logger
 * @brief Source file for SysLog
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_rom_serial_output.h"

#include "Common/All.h"
#include "SysLog.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef ZE_LOG_BUFFER_SIZE
    #define ZE_LOG_BUFFER_SIZE       (256U)
#endif

#ifndef esp_rom_uart_putc
    #define esp_rom_uart_putc(c)     esp_rom_output_putc(c)
#endif
#ifndef esp_rom_uart_flush_tx
    #define esp_rom_uart_flush_tx(u) esp_rom_output_flush_tx(u)
#endif

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

#define ZE_LOG_BUFFER_SIZE (256U)

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

static ze_mutex_t s_sys_log_mutex;
static bool s_sys_log_inited = false;
static char s_sys_log_buf[ZE_LOG_BUFFER_SIZE];

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Initialize system logger driver */
ze_err_t ZE_SysLog_Init(void) {
    ze_err_t err = ZE_Mutex_LockTake(&s_sys_log_mutex, UINT32_MAX);
    if (ZE_SUCCESS != err) {
        return err;
    }

    if (s_sys_log_inited) {
        (void)ZE_Mutex_LockGive(&s_sys_log_mutex);
        return ZE_ERR_ALREADY_INIT;
    }

   esp_rom_output_set_as_console(0U);
    s_sys_log_inited = true;
    (void)ZE_Mutex_LockGive(&s_sys_log_mutex);

    return ZE_SUCCESS;
}

/** @brief Print formatted log message to system console */
ze_err_t ZE_SysLog_Printf(const char *p_fmt, ...) {
    if (NULL == p_fmt) {
        return ZE_ERR_INVALID_ARG;
    }

    ze_err_t err = ZE_Mutex_LockTake(&s_sys_log_mutex, UINT32_MAX);
    if (ZE_SUCCESS != err) {
        return err;
    }

    va_list args;
   va_start(args, p_fmt);
    int len = vsnprintf(s_sys_log_buf, sizeof(s_sys_log_buf), p_fmt, args);
   va_end(args);

    if (len > 0) {
        uint32_t print_len = ((uint32_t)len < sizeof(s_sys_log_buf)) ? (uint32_t)len : (sizeof(s_sys_log_buf) - 1U);
        for (uint32_t i = 0U; i < print_len; i++) {
           esp_rom_uart_putc(s_sys_log_buf[i]);
        }
       esp_rom_uart_flush_tx(0U);
    }

    (void)ZE_Mutex_LockGive(&s_sys_log_mutex);
    return ZE_SUCCESS;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/