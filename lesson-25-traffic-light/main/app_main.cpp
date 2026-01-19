#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include <driver/gptimer.h>
#include <driver/gpio.h>

#include "common.h"
#include "TrafficLight.hpp"
#include "Button.hpp"

void setup_button(TrafficLight* instance);
void setup_timer(TrafficLight* instance);
bool timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);

const TickType_t xDelayMs = 1000 / portTICK_PERIOD_MS;

constexpr const gpio_num_t PIN_BUTTON = static_cast<gpio_num_t>(CONFIG_PIN_USER_BUTTON);
constexpr const gpio_num_t PIN_LED_RED = static_cast<gpio_num_t>(CONFIG_PIN_LED_RED);
constexpr const gpio_num_t PIN_LED_YELLOW = static_cast<gpio_num_t>(CONFIG_PIN_LED_YELLOW);
constexpr const gpio_num_t PIN_LED_GREEN = static_cast<gpio_num_t>(CONFIG_PIN_LED_GREEN);


extern "C" void app_main(void)
{
    auto trafficLight = new TrafficLight();

    trafficLight->init(PIN_LED_RED, PIN_LED_YELLOW, PIN_LED_GREEN);

    setup_button(trafficLight);
    setup_timer(trafficLight);

    for (;;) {
        vTaskDelay(xDelayMs);
    }
}

void setup_button(TrafficLight* instance)
{
    // NB!
    // Not taking care of memory leaks as we not gonna release memory anyway

    auto button = new Button();

    static enum DeviceMode {
        Off,
        Normal,
        Inactive
    }
    s_mode = DeviceMode::Off;

    button->init(PIN_BUTTON, [instance] ()
    {
        switch (s_mode) {
            case DeviceMode::Off:
                s_mode = DeviceMode::Normal;
                instance->set_mode(TrafficLight::Red);
                break;
            case DeviceMode::Normal:
                s_mode = DeviceMode::Inactive;
                instance->set_mode(TrafficLight::Inactive);
                break;
            case DeviceMode::Inactive:
                s_mode = DeviceMode::Off;
                instance->set_mode(TrafficLight::Off);
                break;
        }
    });
}

void setup_timer(TrafficLight* instance)
{
    // NB!
    // Not taking care of handles and stuff as we not gonna release resources anyway

    // Set up 1 us resolution timer
    gptimer_handle_t gptimer = nullptr;
    gptimer_config_t timer_conf = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000,   // 1 us
        .intr_priority = 0,
        .flags = {},
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_conf, &gptimer));

    // Set up 100 ms periodic alarm
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = TrafficLight::TICK_MS * 1000,
        .reload_count = 0,                              // When the alarm event occurs, the timer will automatically reload to 0
        .flags = { .auto_reload_on_alarm = true },      // Enable auto-reload function
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = { .on_alarm = timer_cb };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, instance));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

bool timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    static_cast<TrafficLight*>(user_ctx)->process_state();

    return false;
}
