

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h" /* IWYU pragma: keep (must be included before task.h) */
#include "freertos/task.h"
#include "driver/gpio.h"
#include "Common/SharedAPIs.h"

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

#define MAIN_LED_GPIO      (GPIO_NUM_2)
#define MAIN_LED_TOGGLE_MS (500U)

/* PRIVATE VARIABLES *****************************************************************************************
 ************************************************************************************************************/

static const char *sp_tag = "MAIN";

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

/** @brief Application entry point: report startup status, then blink the on-board LED (GPIO2) */
void app_main(void) {
    zecu_err_t err = ZECU_OK;
    uint32_t blink_cnt = 0U;
    uint32_t led_level = 0U;

    ESP_LOGI(sp_tag, "Zonal ECU Firmware v2 starting...");
    ESP_LOGI(sp_tag, "Sample error check: code %d -> %s", err, ZECU_ErrToStr(err));

    err = gpio_reset_pin(MAIN_LED_GPIO);

    /* Stop here if the LED pin cannot be reset */
    if (ZECU_OK != err) {
        ESP_LOGE(sp_tag, "gpio_reset_pin failed: %s", ZECU_ErrToStr(err));

        /* Nothing else to run without the LED */
        return;
    }

    err = gpio_set_direction(MAIN_LED_GPIO, GPIO_MODE_OUTPUT);

    /* Stop here if the LED pin cannot be configured as output */
    if (ZECU_OK != err) {
        ESP_LOGE(sp_tag, "gpio_set_direction failed: %s", ZECU_ErrToStr(err));

        /* Nothing else to run without the LED */
        return;
    }

    /* Toggle the LED forever as a heartbeat */
    while (true) {
        led_level = blink_cnt % 2U;
        (void)gpio_set_level(MAIN_LED_GPIO, led_level);
        ESP_LOGW(sp_tag, "LED %s (count=%lu)", (0U != led_level) ? "ON" : "OFF", (unsigned long)blink_cnt);
        blink_cnt++;
        vTaskDelay(pdMS_TO_TICKS(MAIN_LED_TOGGLE_MS));
    }
}

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
