#pragma once

#include "LED.hpp"

#include <atomic>

class TrafficLight
{
public:
    static const int TICK_MS = 100;
    static const int LIGHT_DURATION_TICKS = 30;
    static const int BLINK_DURATION_TICKS = 5;

    enum Mode {
        Off,
        Red,
        RedYellow,
        Green,
        GreenBlink,
        Yellow,
        YellowBlink,
        Inactive = YellowBlink,
    };

    TrafficLight();

    // Timer tick in msec
    void init(gpio_num_t pin_led_red, gpio_num_t pin_led_yellow, gpio_num_t pin_led_green);
    void set_mode(Mode mode);
    void process_state();

private:
    const int RED = 0;
    const int YELLOW = 1;
    const int GREEN = 2;

    int m_ticks = 0;
    int m_blink_counter = 0;
    Mode m_mode = Mode::Off;
    LED  m_leds[3];

    bool tick_and_check_timeout();
    void set_lights(bool red, bool yellow, bool green);
};

