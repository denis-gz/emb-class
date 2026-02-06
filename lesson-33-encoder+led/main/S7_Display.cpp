#include "S7_Display.h"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <cctype>

#include "common.h"

S7_Display::S7_Display(size_t num_digits)
    : m_num_digits(num_digits)
{
    // The caller should configure each digit individually using subscript operator[]
    // on this object
    m_digits = new S7_Digit[num_digits];

    S7_Digit::digit_config_t digit_config = {
        .active_level = 0,
        .pin_A  = CONFIG_PIN_LED_SEG_A,
        .pin_B  = CONFIG_PIN_LED_SEG_B,
        .pin_C  = CONFIG_PIN_LED_SEG_C,
        .pin_D  = CONFIG_PIN_LED_SEG_D,
        .pin_E  = CONFIG_PIN_LED_SEG_E,
        .pin_F  = CONFIG_PIN_LED_SEG_F,
        .pin_G  = CONFIG_PIN_LED_SEG_G,
        .pin_DP = CONFIG_PIN_LED_SEG_DP,
    };

    gpio_config_t gpio_conf = {
        .pin_bit_mask = digit_config.bit_mask()
            | (1 << CONFIG_PIN_LED_DIG1)
            | (1 << CONFIG_PIN_LED_DIG2)
            | (1 << CONFIG_PIN_LED_DIG3),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&gpio_conf);

    digit_config.pin_COM = CONFIG_PIN_LED_DIG3,
    m_digits[0].SetConfig(digit_config);

    digit_config.pin_COM = CONFIG_PIN_LED_DIG2,
    m_digits[1].SetConfig(digit_config);

    digit_config.pin_COM = CONFIG_PIN_LED_DIG1,
    m_digits[2].SetConfig(digit_config);

    m_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 10 * 1000,          // 10 kHz - 1 tick equals 100 us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&m_timer_config, &m_timer));

    // Configure periodic alarm to redraw digits (1 kHz refresh rate)
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = m_timer_config.resolution_hz / 1000,    // 1 ms period
        .reload_count = 0,
        .flags { .auto_reload_on_alarm = 1 }
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(m_timer, &alarm_config));

    gptimer_event_callbacks_t cbs = { .on_alarm = on_timer_tick };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(m_timer, &cbs, this));
    ESP_ERROR_CHECK(gptimer_enable(m_timer));
}

S7_Display::~S7_Display()
{
    for (size_t i = 0; i < m_num_digits; ++i) {
        m_digits[i].Hide();
    }
    gptimer_disable(m_timer);
    gptimer_del_timer(m_timer);

    delete[] m_digits;
}

void S7_Display::Start()
{
    gptimer_start(m_timer);
}

void S7_Display::Stop()
{
    gptimer_stop(m_timer);
}

void S7_Display::Print(const char* text)
{
    for (int i = 0; i < m_num_digits; ++i) {
        if (isdigit(text[i])) {
            m_digits[m_num_digits-i-1].SetSegments(S7_Digit::DIGIT[text[i] - 0x30]);
        }
        else if (int ch = toupper(text[i]) - 0x41; ch < countof(S7_Digit::LETTER)) {
            m_digits[m_num_digits-i-1].SetSegments(S7_Digit::LETTER[ch]);
        }
        else {
            m_digits[m_num_digits-i-1].SetSegments(S7_Digit::SEG_D);
        }
    }
}

void S7_Display::Print(int n, int point_position)
{
    bool is_neg = (n < 0);
    if (is_neg)
        n = -n;
    bool skip_rest = (n == 0);
    for (int i = 0; i < m_num_digits; ++i) {
        auto point = (i == point_position) ? S7_Digit::SEG_DP : S7_Digit::None;
        if (skip_rest) {
            if (is_neg) {
                is_neg = false;
                m_digits[i].SetSegments(S7_Digit::SEG_G);
            }
            else {
                m_digits[i].SetSegments(point | S7_Digit::None);
            }
        }
        else {
            m_digits[i].SetSegments(point | S7_Digit::DIGIT[n % 10]);
            skip_rest = !(n /= 10);
        }
    }
}

IRAM_ATTR bool S7_Display::on_timer_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    auto& _ = *static_cast<S7_Display*>(user_ctx);

    _.m_digits[_.m_index++ % _.m_num_digits].Hide();
    _.m_digits[_.m_index   % _.m_num_digits].Refresh();

    return false;
}
