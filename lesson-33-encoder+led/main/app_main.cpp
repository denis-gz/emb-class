#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/pulse_cnt.h>
#include <driver/gpio.h>

#include "common.h"

#include "Calculator.h"

extern "C" void app_main(void)
{
    Calculator calc(3);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

