/********************************************************************************************************************************
 * @file Startup.c
 * @ingroup ZonalECU_FWv2
 * @brief Source file for [[Startup]]. Implements BareMetal driver setup and RTOS services dispatcher.
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "Startup.h"
#include "driver/gpio.h"
#include "Comm/ESP-Logger/SysLog.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define MAIN_HEARTBEAT_LED_GPIO (GPIO_NUM_2)

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Initialize low-level bare-metal hardware and drivers before RTOS services
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_BareMetal_Start(void) {
   ZE_LOGI("Zonal ECU Firmware v2: CALLED");
    return ZE_SUCCESS;
}

/** @brief   Spawn and start application FreeRTOS services
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_Start(void) {
    ze_err_t err;

   ZE_LOGI("Zonal ECU Firmware v2: Starting RTOS services...");

    err = ZE_Srv_Init();
    if (ZE_SUCCESS != err) {
       ZE_LOGE("Failed to initialize ZE-Services (err %d)", err);
        return err;
    }

   ZE_LOGI("All enabled RTOS services launched successfully");
    return ZE_SUCCESS;
}

/** @brief   Stop BareMetal services/hardware safely before system shutdown or exit
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_BareMetal_Stop(void) {
   ZE_LOGW("Zonal ECU Firmware v2: Stopping BareMetal hardware/services...");
    return ZE_SUCCESS;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/