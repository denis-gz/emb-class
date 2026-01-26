#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "common.h"

const TickType_t xDelayMs = 1000 / portTICK_PERIOD_MS;

extern "C" void app_main(void)
{
	for (uint32_t cnt = 0;; cnt++) {
        ESP_LOGI(TAG, "");

        if (cnt % 3 == 0) {
        }

        vTaskDelay(xDelayMs);
    }
}

