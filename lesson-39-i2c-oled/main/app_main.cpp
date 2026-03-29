#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <memory>

#include "EnvironmentMonitor.h"

extern "C" void app_main(void)
{
    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    auto monitor = std::make_unique<EnvironmentMonitor>();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

