#pragma once

#include <cstddef>
#include <driver/gptimer.h>

#include "S7_Digit.h"

class S7_Display
{
public:
    S7_Display(size_t num_digits);
    ~S7_Display();

    S7_Digit& operator[](size_t index) { return m_digits[index]; }

    void Start();
    void Stop();

    void Print(const char* text);
    void Print(int n, int point_position = -1);

private:
    static bool on_timer_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);
    const size_t m_num_digits;

    S7_Digit* m_digits = nullptr;
    size_t m_index = 0;

    gptimer_handle_t m_timer = nullptr;
    gptimer_config_t m_timer_config = {};
    gptimer_alarm_config_t m_alarm_config = {};
};

