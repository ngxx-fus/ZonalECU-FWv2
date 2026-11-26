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
    esp_err_t ret;

   ZE_LOGI("Zonal ECU Firmware v2: Starting BareMetal initialization...");

    ret = gpio_reset_pin(MAIN_HEARTBEAT_LED_GPIO);
    if (ESP_OK != ret) {
       ZE_LOGE("Failed to reset heartbeat LED GPIO (err %d)", ret);
        return ZE_ERR_UNEXPECTED;
    }

    ret = gpio_set_direction(MAIN_HEARTBEAT_LED_GPIO, GPIO_MODE_OUTPUT);
    if (ESP_OK != ret) {
       ZE_LOGE("Failed to set direction for heartbeat LED GPIO (err %d)", ret);
        return ZE_ERR_UNEXPECTED;
    }

   ZE_LOGI("BareMetal initialization completed successfully");
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

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/