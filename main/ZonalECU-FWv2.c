#include "freertos/FreeRTOS.h" // IWYU pragma: keep (phải include trước task.h)
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "template.h"
#include "subdir/template.h"

#define LED_GPIO    GPIO_NUM_2

static const char *TAG = "ZonalECU";

void app_main(void)
{
    template_func();
    subdir_template_func();

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    uint32_t count = 0;
    while (1) {
        gpio_set_level(LED_GPIO, count % 2);
        ESP_LOGW(TAG, "LED %s (count=%lu)", (count % 2) ? "ON" : "OFF", count);
        count++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
