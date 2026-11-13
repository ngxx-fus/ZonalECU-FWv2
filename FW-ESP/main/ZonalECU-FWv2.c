

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h" /* IWYU pragma: keep (must be included before task.h) */
#include "freertos/task.h"
#include "driver/gpio.h"
#include "Common/All.h"
#include "Comm/ESP-Logger/All.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define MAIN_LED_GPIO      (GPIO_NUM_2)
#define MAIN_LED_TOGGLE_MS (500U)

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

/* FUNCTION IMPLEMENTATIONS ******************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Application entry point: report startup status, then blink the on-board LED (GPIO2) */
void app_main(void) {
    ze_err_t err = ZE_OK;
    uint32_t blink_cnt = 0U;
    uint32_t led_level = 0U;

    ZE_LOGI("Zonal ECU Firmware v2 starting...");
    ZE_LOGI("Sample error check: code %d -> %s", err, ZE_ErrToStr(err));

    err = gpio_reset_pin(MAIN_LED_GPIO);

    /* Stop here if the LED pin cannot be reset */
    if (ZE_OK != err) {
        ZE_LOGE("gpio_reset_pin failed: %s", ZE_ErrToStr(err));

        /* Nothing else to run without the LED */
        return;
    }

    err = gpio_set_direction(MAIN_LED_GPIO, GPIO_MODE_OUTPUT);

    /* Stop here if the LED pin cannot be configured as output */
    if (ZE_OK != err) {
        ZE_LOGE("gpio_set_direction failed: %s", ZE_ErrToStr(err));

        /* Nothing else to run without the LED */
        return;
    }

    /* Toggle the LED forever as a heartbeat */
    while (true) {
        led_level = blink_cnt % 2U;
        (void)gpio_set_level(MAIN_LED_GPIO, led_level);
        ZE_LOGW("LED %s (count=%lu)", (0U != led_level) ? "ON" : "OFF", (unsigned long)blink_cnt);
        blink_cnt++;
        vTaskDelay(pdMS_TO_TICKS(MAIN_LED_TOGGLE_MS));
    }
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/