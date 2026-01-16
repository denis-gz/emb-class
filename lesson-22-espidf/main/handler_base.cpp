#include "handler_base.hpp"

#include <freertos/FreeRTOS.h>

#include <esp_timer.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "common.h"

constexpr const gpio_num_t PIN_BUTTON = static_cast<gpio_num_t>(CONFIG_PIN_USER_BUTTON);

HandlerBase::HandlerBase(const char* tag)
    : m_tag(tag)
{ }

HandlerBase::~HandlerBase()
{
    gpio_config_t gpio_conf = {};
    gpio_conf.pin_bit_mask = 1ULL << PIN_BUTTON,
    gpio_config(&gpio_conf);

    gpio_intr_disable(PIN_BUTTON);
    gpio_isr_handler_remove(PIN_BUTTON);
    gpio_uninstall_isr_service();

    if (gl_handle) {
        gpio_glitch_filter_disable(gl_handle);
        gpio_del_glitch_filter(gl_handle);
    }
}

esp_err_t HandlerBase::Init(gpio_int_type_t int_type)
{
    esp_err_t ret;

    gpio_config_t gpio_conf = {
        .pin_bit_mask = 1ULL << PIN_BUTTON,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = int_type,
    };

    // Convert pointer-to-member to a C-function with an arg (gpio_isr_t)
    union {
        void (HandlerBase::*member)();
        gpio_isr_t gpio_isr;
    }
    handler = {
        .member = &HandlerBase::Handler
    };

    ESP_ERROR_CHECK(ret = gpio_config(&gpio_conf));
    ESP_ERROR_CHECK(ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(ret = gpio_isr_handler_add(PIN_BUTTON, handler.gpio_isr, this));
    ESP_ERROR_CHECK(ret = gpio_intr_enable(PIN_BUTTON));

    gpio_pin_glitch_filter_config_t gl_conf = {
        .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
        .gpio_num = PIN_BUTTON,
    };

    ESP_ERROR_CHECK(ret = gpio_new_pin_glitch_filter(&gl_conf, &gl_handle));
    ESP_ERROR_CHECK(ret = gpio_glitch_filter_enable(gl_handle));

    int64_t now_time = esp_timer_get_time();
    m_depress_time = now_time;
    m_release_time = now_time;
    m_level = 1;

    return ret;
}

void IRAM_ATTR HandlerBase::Handler()
{
    int64_t now_time = esp_timer_get_time();
    int level = gpio_get_level(PIN_BUTTON);

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

    HandlerImpl();

    // Save for logging
    if (m_index < countof(m_buffer)) {
        m_buffer[m_index++] = std::make_tuple(
            xPortGetCoreID(),             // #0
            (int64_t) m_sample_time,      // #1
            (int64_t) m_depress_time,     // #2
            (int64_t) m_release_time,     // #3
            (int) m_level,                // #4
            (int) m_counter               // #5
        );
    }
}

bool HandlerBase::LogSamples()
{
    if (m_index) {
        for (int i = 0; i < m_index; i++) {
            const auto& r = m_buffer[i > 0 ? i - 1 : 0];
            const auto& s = m_buffer[i];
            ESP_LOGI(m_tag, "[core%d] %lld us: level %d (+% 7lld us), depress: %lld us, release: %lld us, counter: %d",
                std::get<0>(s),           // xPortGetCoreID
                std::get<1>(s),           // m_sample_time
                std::get<4>(s),           // m_level
                get<1>(s) - get<1>(r),    // time since last sample
                std::get<2>(s),           // m_depress_time
                std::get<3>(s),           // m_release_time
                std::get<5>(s)            // m_counter
            );
        }
        m_index = 0;
        return true;
    }
    else {
        return false;
    }
}
