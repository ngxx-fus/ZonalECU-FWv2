/********************************************************************************************************************************
 * @file ZonalECU-FWv2.c
 * @ingroup ZonalECU_FWv2
 * @brief Application entry point for Zonal ECU Firmware v2.
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdlib.h>
#include "esp_system.h"
#include "esp_private/startup_internal.h"
#include "Comm/ESP-Logger/SysLog.h"
#include "App/ZE-Services/Startup.h"

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Wrapper hook for atexit() and esp_register_shutdown_handler() */
static void prv_atexit_handler(void) {
    ze_err_t err = ZE_BareMetal_Stop();
    if (ZE_SUCCESS != err) {
        ZE_LOGE("BareMetal stop failed (err %d)", err);
    }
}

/** @brief   Pre-RTOS hardware and driver initialization hook executed before FreeRTOS scheduler starts
 *  @return  ESP_OK on success, or ESP_FAIL to abort boot */
ESP_SYSTEM_INIT_FN(ze_baremetal_init, SECONDARY, BIT(0), 999) {
    (void)atexit(prv_atexit_handler);
    (void)esp_register_shutdown_handler(prv_atexit_handler);

    ZE_LOGI("CALL: ZE_BareMetal_Start()");
    ze_err_t err = ZE_BareMetal_Start();
    if (ZE_SUCCESS != err) {
        ZE_LOGE("BareMetal startup aborted (err %d)", err);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Application main entry point */
void app_main(void) {
    ze_err_t err;

    ZE_LOGI("CALL: ZE_RTOS_Start()");
    err = ZE_RTOS_Start();
    if (ZE_SUCCESS != err) {
        ZE_LOGE("RTOS startup aborted (err %d)", err);
    }

    vTaskDelete(NULL);
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/
