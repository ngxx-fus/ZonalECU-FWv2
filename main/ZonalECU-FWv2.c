#include <stdio.h>
#include "Common/SharedAPIs.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Zonal ECU Firmware v2 starting...");

    zecu_err_t err = ZECU_OK;
    ESP_LOGI(TAG, "Sample error check: code %d -> %s", err, ZECU_ErrToStr(err));
}
