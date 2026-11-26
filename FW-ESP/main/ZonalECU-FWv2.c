/********************************************************************************************************************************
 * @file ZonalECU-FWv2.c
 * @ingroup ZonalECU_FWv2
 * @brief Application entry point for Zonal ECU Firmware v2.
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "Common/All.h"
#include "Comm/ESP-Logger/SysLog.h"
#include "App/ZE-Services/Startup.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define MAIN_HEARTBEAT_LED_GPIO (GPIO_NUM_2)
#define MAIN_HEARTBEAT_DELAY_MS (500U)

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Application main entry point */
void app_main(void) {
    ze_err_t err;
    uint32_t toggle = 0U;

    err = ZE_BareMetal_Start();
    if (ZE_SUCCESS != err) {
       ZE_LOGE("BareMetal startup aborted (err %d)", err);
        return;
    }

    err = ZE_RTOS_Start();
    if (ZE_SUCCESS != err) {
       ZE_LOGE("RTOS startup aborted (err %d)", err);
        return;
    }

    /* Heartbeat loop in background task */
    while (true) {
        (void)gpio_set_level(MAIN_HEARTBEAT_LED_GPIO, toggle & 1U);
        toggle++;
       vTaskDelay(pdMS_TO_TICKS(MAIN_HEARTBEAT_DELAY_MS));
    }
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/