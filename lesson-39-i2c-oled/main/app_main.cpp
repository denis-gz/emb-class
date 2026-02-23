#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <memory>

#include "EnvironmentMonitor.h"

extern "C" void app_main(void)
{
    auto monitor = std::make_unique<EnvironmentMonitor>();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

