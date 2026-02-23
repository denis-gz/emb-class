#pragma once

#include <cstddef>
#include <vector>

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

    void PrintNumber(int n, int input_position = -1);
    int PrintText(const char* text, int len = -1);
    int PrintSegments(const uint8_t* segments, size_t count);
    int StartAnimation(const uint8_t* segments, size_t count);

    void PrintTest();
    void PrintWaitIndicator();
    void SetAnimationTimings(int period, int duration = INT32_MAX);

    bool IsAnimationRunning() const { return m_shifter_mode != None; }

    size_t NumDigits() const { return m_num_digits; }

private:
    static bool on_switcher_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);
    static bool on_shifter_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);
    static uint8_t char_to_segments(char ch);

    enum ShifterMode {
        None,
        Scroll,
        Frame,
    };

    const size_t m_num_digits;

    S7_Digit* m_digits = nullptr;
    size_t m_switch_index = 0;
    size_t m_shift_index = 0;
    ShifterMode m_shifter_mode = {};

    int m_animation_period = 1;             // In timer units (100 ms)
    int m_animation_duration = INT32_MAX;   // In timer units (100 ms)

    gptimer_handle_t m_switch_timer = nullptr;
    gptimer_handle_t m_shift_timer = nullptr;
    uint64_t m_shifter_counter = 0;

    std::vector<uint8_t> m_segs_buffer;
};

