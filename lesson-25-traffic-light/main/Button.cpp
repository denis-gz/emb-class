#include "Button.hpp"

#include <freertos/FreeRTOS.h>

#include <esp_timer.h>
#include <esp_log.h>

// How much time a pin should have the HIGH level to clear button depressed state
constexpr const int64_t HIGH_STATE_GUARD_INTERVAL_US = 10 * 1000LL;    // 10 ms

Button::Button()
{ }

Button::~Button()
{
    if (m_pin != GPIO_NUM_NC) {
        gpio_reset_pin(m_pin);
        gpio_isr_handler_remove(m_pin);
    }
    if (gl_handle) {
        gpio_glitch_filter_disable(gl_handle);
        gpio_del_glitch_filter(gl_handle);
    }
}

esp_err_t Button::init(gpio_num_t pin, callback_t callback)
{
    esp_err_t ret;

    gpio_config_t gpio_conf = {
        .pin_bit_mask = 1ULL << pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };

    // Convert pointer-to-member to a C-function with an arg (gpio_isr_t)
    union {
        void (Button::*member)();
        gpio_isr_t gpio_isr;
    }
    handler = {
        .member = &Button::handler
    };

    ESP_ERROR_CHECK(ret = gpio_config(&gpio_conf));
    ESP_ERROR_CHECK(ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(ret = gpio_isr_handler_add(pin, handler.gpio_isr, this));
    ESP_ERROR_CHECK(ret = gpio_intr_enable(pin));
    m_pin = pin;
    m_callback = callback;

    gpio_pin_glitch_filter_config_t gl_conf = {
        .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
        .gpio_num = pin,
    };

    ESP_ERROR_CHECK(ret = gpio_new_pin_glitch_filter(&gl_conf, &gl_handle));
    ESP_ERROR_CHECK(ret = gpio_glitch_filter_enable(gl_handle));

    // Set appripriate defaults so we don't miss the first click
    int64_t now_time = esp_timer_get_time();
    m_depress_time = now_time;
    m_release_time = now_time;
    m_level = 1;

    return ret;
}

void IRAM_ATTR Button::handler()
{
    int64_t now_time = esp_timer_get_time();
    int level = gpio_get_level(m_pin);

    // Check if pin level is unchanged
    if (m_level == level) {
        if (level)                  // High level - button remains released
            m_depress_time = 0;
        else
            m_release_time = 0;     // Low level - button remains depressed
    }
    else {                          // Level changed
        if (level)
            m_release_time = now_time;
        else
            m_depress_time = now_time;
    }

    m_level = level;                // Store actual values
    m_sample_time = now_time;

    if (m_level == 0
        && m_depress_time   // Exclude consequtive depress or release events
        && m_release_time
        && m_depress_time - m_release_time > HIGH_STATE_GUARD_INTERVAL_US) {
        m_callback();
    }
}
