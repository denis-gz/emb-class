#pragma once

#include "Button.h"
#include "RotaryEncoder.h"
#include "S7_Display.h"

#include <functional>
#include <ctime>

class ClockAdjuster
{
    enum state_t {
        Wait,
        Year,
        Month,
        Day,
        WeekDay,
        Hour,
        Minute,
    };

public:
    using callback_t = std::function<void(tm*)>;

    ClockAdjuster(callback_t get_callback, callback_t set_callback);

    S7_Display* Display() { return &m_display; };

private:
    state_t m_state = {};
    tm m_time_info = {};
    S7_Display m_display;
    callback_t m_get_callback;
    callback_t m_set_callback;

    RotaryEncoder m_encoder;
    Button m_encoder_key;

    static void get_day_of_week(int wday, char* result, size_t len);

    void on_rotate(bool increase);
    void on_click();
    void update_display();
};
