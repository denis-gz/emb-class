#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "no_debounce.hpp"
#include "time_based.hpp"
#include "state_based.hpp"

typedef StateBased ImplClass;

const TickType_t xDelayMs = 1000 / portTICK_PERIOD_MS;

extern "C" void app_main(void)
{
    // Doesn't work on stack for some reason...
    auto impl = new ImplClass();

    impl->Init();
    ESP_LOGI(impl->Tag(), "Waiting for button click...");

    for (uint32_t cnt = 0;; cnt++) {
        ESP_LOGI(impl->Tag(), "%d", impl->GetCounter());

        if (cnt % 3 == 0) {
            impl->LogSamples();
        }

        vTaskDelay(xDelayMs);
    }
}

