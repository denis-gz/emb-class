#include "S7_Display.h"

#include <cctype>
#include <cstring>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>

#include "common.h"

S7_Display::S7_Display(size_t num_digits)
    : m_num_digits(num_digits)
{
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

    {   // Timer configuration for switching of digits (1 kHz scan rate)
        gptimer_config_t switcher_config = {
            .clk_src = GPTIMER_CLK_SRC_XTAL,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 10 * 1000,          // 10 kHz - 1 tick equals 100 us
        };
        ESP_ERROR_CHECK(gptimer_new_timer(&switcher_config, &m_switch_timer));

        gptimer_alarm_config_t switcher_alarm_config = {
            .alarm_count = 10,                   // 1 ms period
            .reload_count = 0,
            .flags { .auto_reload_on_alarm = 1 }
        };
        ESP_ERROR_CHECK(gptimer_set_alarm_action(m_switch_timer, &switcher_alarm_config));

        gptimer_event_callbacks_t cbs = { .on_alarm = on_switcher_tick };
        ESP_ERROR_CHECK(gptimer_register_event_callbacks(m_switch_timer, &cbs, this));
        ESP_ERROR_CHECK(gptimer_enable(m_switch_timer));
    }

    {   // Timer configuration for shifting symbols of a sequence (used to scroll longer
        // text or do animations)
        gptimer_config_t shifter_config = {
            .clk_src = GPTIMER_CLK_SRC_XTAL,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 1000,               // 1000 Hz - 1 tick equals 1 ms
        };
        ESP_ERROR_CHECK(gptimer_new_timer(&shifter_config, &m_shift_timer));

        // Configure periodic alarm to redraw digits (1 kHz refresh rate)
        gptimer_alarm_config_t shifter_alarm_config = {
            .alarm_count = 100,                  // 100 ms period
            .reload_count = 0,
            .flags { .auto_reload_on_alarm = 1 }
        };
        ESP_ERROR_CHECK(gptimer_set_alarm_action(m_shift_timer, &shifter_alarm_config));

        gptimer_event_callbacks_t cbs = { .on_alarm = on_shifter_tick };
        ESP_ERROR_CHECK(gptimer_register_event_callbacks(m_shift_timer, &cbs, this));
        ESP_ERROR_CHECK(gptimer_enable(m_shift_timer));
    }
}

S7_Display::~S7_Display()
{
    gptimer_disable(m_shift_timer);
    gptimer_del_timer(m_shift_timer);

    gptimer_disable(m_switch_timer);
    gptimer_del_timer(m_switch_timer);

    for (size_t i = 0; i < m_num_digits; ++i) {
        m_digits[i].Hide();
    }

    delete[] m_digits;
}

void S7_Display::Start()
{
    gptimer_start(m_switch_timer);
    gptimer_start(m_shift_timer);
}

void S7_Display::Stop()
{
    gptimer_stop(m_shift_timer);
    gptimer_stop(m_switch_timer);
}

/* static */ uint8_t S7_Display::char_to_segments(char ch)
{
    if (isdigit(ch)) {
        return S7_Digit::DIGIT[ch - 0x30];
    }
    else if (int uch = toupper(ch) - 0x41; uch < countof(S7_Digit::LETTER)) {
        return S7_Digit::LETTER[uch];
    }
    else if (ch == ' ') {
        return S7_Digit::None;
    }
    else if (ch == '-') {
        return S7_Digit::SEG_G;
    }
    else {
        return S7_Digit::SEG_D;
    }
}

void S7_Display::PrintNumber(int n, int input_position)
{
    gptimer_stop(m_shift_timer);
    m_shifter_mode = None;
    m_shift_index = 0;
    m_segs_buffer.resize(0);

    bool is_neg = (n < 0);
    if (is_neg)
        n = -n;

    int k = 0;
    for (int m = n; m; k++)
        m /= 10;
    if (k > m_num_digits || (is_neg && k > m_num_digits - 1)) {
        PrintText("---");
        return;
    }

    bool skip_rest = (n == 0);
    for (int i = 0; i < m_num_digits; ++i) {
        if (skip_rest) {
            if (i <= input_position) {
                m_digits[i].SetSegments(S7_Digit::DIGIT[0]);
            }
            else if (is_neg) {
                is_neg = false;
                m_digits[i].SetSegments(S7_Digit::SEG_G);
            }
            else {
                m_digits[i].SetSegments(S7_Digit::None);
            }
        }
        else {
            m_digits[i].SetSegments(S7_Digit::DIGIT[n % 10]);
            skip_rest = !(n /= 10);
        }
    }
}

int S7_Display::PrintText(const char* text, int len)
{
    int duration_ms;
    gptimer_stop(m_shift_timer);

    if (len == -1)
        len = std::strlen(text);
    if (len > m_num_digits) {
        m_shift_index = 0;
        m_shifter_mode = Scroll;
        m_segs_buffer.resize(len);
        for (int i = 0; i < len; ++i) {
            m_segs_buffer[i] = char_to_segments(text[i]);
        }
        // Compute duration so animation stops at the last symbol
        if (m_animation_duration == -1)
            m_animation_duration = len - m_num_digits + 1;
        duration_ms = m_animation_duration * m_animation_period * 100;
        gptimer_set_raw_count(m_shift_timer, 1);
        gptimer_start(m_shift_timer);
    }
    else {
        m_shifter_mode = None;
        m_segs_buffer.resize(0);
        for (int i = 0; i < m_num_digits; ++i) {
            // Digits indexing go the opposite way to text
            int j = m_num_digits-i-1;
            m_digits[j].SetSegments(char_to_segments(text[i]));
        }
        duration_ms = 0;
    }
    return duration_ms;
}

int S7_Display::PrintSegments(const uint8_t* segments, size_t count)
{
    int duration_ms;
    gptimer_stop(m_shift_timer);

    if (count > m_num_digits) {
        m_shift_index = 0;
        m_shifter_mode = Scroll;
        m_segs_buffer.resize(count);
        std::memcpy(m_segs_buffer.data(), segments, count);
        duration_ms = m_animation_duration * m_animation_period * 100;
        gptimer_set_raw_count(m_shift_timer, 1);
        gptimer_start(m_shift_timer);
    }
    else {
        m_shifter_mode = None;
        m_segs_buffer.resize(0);
        for (size_t i = 0; i < count; ++i) {
            int j = m_num_digits-i-1;
            m_digits[j].SetSegments(segments[i]);
        }
        duration_ms = 0;
    }
    return duration_ms;
}

int S7_Display::StartAnimation(const uint8_t* segments, size_t count)
{
    gptimer_stop(m_shift_timer);
    m_shift_index = 0;
    m_shifter_mode = Frame;
    m_segs_buffer.resize(count);
    std::memcpy(m_segs_buffer.data(), segments, count);
    gptimer_set_raw_count(m_shift_timer, 1);
    gptimer_start(m_shift_timer);

    return m_animation_duration * m_animation_period * 100;
}

void S7_Display::PrintTest()
{
    constexpr static uint8_t SEGMENTS[] = {
        S7_Digit::SEG_A,
        S7_Digit::SEG_B,
        S7_Digit::SEG_C,
        S7_Digit::SEG_D,
        S7_Digit::SEG_E,
        S7_Digit::SEG_F,
        S7_Digit::SEG_G,
        S7_Digit::SEG_DP,
        S7_Digit::None,
    };

    auto test_pattern = (uint8_t*) alloca(m_num_digits * sizeof(SEGMENTS));
    for (size_t i = 0; i < countof(SEGMENTS); ++i) {
        for (size_t j = 0; j < m_num_digits; ++j) {
            test_pattern[i * m_num_digits + j] = SEGMENTS[i];
        }
    }

    SetAnimationTimings(2, countof(SEGMENTS));
    StartAnimation(test_pattern, m_num_digits * sizeof(SEGMENTS));
}

void S7_Display::PrintWaitIndicator()
{
    constexpr static uint8_t TEST_PATTERN[] = {
        S7_Digit::SEG_A, S7_Digit::None, S7_Digit::SEG_D,
        S7_Digit::None, S7_Digit::SEG_A | S7_Digit::SEG_D, S7_Digit::None,
        S7_Digit::SEG_D, S7_Digit::None, S7_Digit::SEG_A,
        S7_Digit::SEG_E, S7_Digit::None, S7_Digit::SEG_B,
        S7_Digit::SEG_F, S7_Digit::None, S7_Digit::SEG_C,
    };

    StartAnimation(TEST_PATTERN, sizeof(TEST_PATTERN));
}

void S7_Display::SetAnimationTimings(int period, int duration)
{
    m_animation_period = period;
    m_animation_duration = duration;
}

IRAM_ATTR bool S7_Display::on_switcher_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    auto& _ = *static_cast<S7_Display*>(user_ctx);

    _.m_digits[_.m_switch_index++ % _.m_num_digits].Hide();
    _.m_digits[_.m_switch_index   % _.m_num_digits].Refresh();

    return false;
}

IRAM_ATTR bool S7_Display::on_shifter_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    auto& _ = *static_cast<S7_Display*>(user_ctx);

    do {
        // Skip when no segments to display
        if (_.m_segs_buffer.empty())
            break;
        // Skip when duration is over
        if (!_.m_animation_duration)
            break;
        // Skip when period has not yet elapsed
        if (++_.m_shifter_counter % _.m_animation_period)
            break;

        // Take `m_num_digits' segments starting at `m_shift_index' position and
        // write them to the display buffer in reverse order
        for (size_t i = 0; i < _.m_num_digits; ++i) {
            int j = _.m_num_digits-i-1;
            int k = (_.m_shift_index + i) % _.m_segs_buffer.size();
            _.m_digits[j].SetSegments(_.m_segs_buffer[k]);
        }

        --_.m_animation_duration;
        if (_.m_shifter_mode == Scroll)
            ++_.m_shift_index;
        else // Frame
            _.m_shift_index += _.m_num_digits;
    }
    while (0);

    return false;
}
