#include "TrafficLight.hpp"


TrafficLight::TrafficLight()
{ }

void TrafficLight::init(gpio_num_t pin_led_red, gpio_num_t pin_led_yellow, gpio_num_t pin_led_green)
{
    m_leds[RED].init(pin_led_red);
    m_leds[YELLOW].init(pin_led_yellow);
    m_leds[GREEN].init(pin_led_green);

    set_mode(Mode::Off);
}

void TrafficLight::set_mode(Mode mode)
{
    m_mode = mode;

    switch (m_mode) {
        case Mode::Off:
            set_lights(0, 0, 0);
            m_ticks = 0;
            break;
        case Mode::Red:
            set_lights(1, 0, 0);
            m_ticks = LIGHT_DURATION_TICKS;
            break;
        case Mode::RedYellow:
            set_lights(1, 1, 0);
            m_ticks = LIGHT_DURATION_TICKS;
            break;
        case Mode::Green:
            set_lights(0, 0, 1);
            m_ticks = LIGHT_DURATION_TICKS;
            break;
        case Mode::GreenBlink:
            set_lights(0, 0, !m_leds[GREEN].get_status());
            m_ticks = BLINK_DURATION_TICKS;
            break;
        case Mode::Yellow:
            set_lights(0, 1, 0);
            m_ticks = LIGHT_DURATION_TICKS;
            break;
        case Mode::YellowBlink:
            set_lights(0, !m_leds[YELLOW].get_status(), 0);
            m_ticks = BLINK_DURATION_TICKS;
            break;
    }
}

void TrafficLight::process_state()
{
    switch (m_mode) {
        case Mode::Off:
            break;
        case Mode::Inactive:
            if (tick_and_check_timeout())
                set_mode(Mode::YellowBlink);
            break;
        case Mode::Red:
            if (tick_and_check_timeout())
                set_mode(Mode::RedYellow);
            break;
        case Mode::RedYellow:
            if (tick_and_check_timeout())
                set_mode(Mode::Green);
            break;
        case Mode::Green:
            if (tick_and_check_timeout()) {
                m_blink_counter = 6;
                set_mode(Mode::GreenBlink);
            }
            break;
        case Mode::GreenBlink:
            if (tick_and_check_timeout()) {
                if (--m_blink_counter) {
                    set_mode(Mode::GreenBlink);
                }
                else {
                    set_mode(Mode::Yellow);
                }
            }
            break;
        case Mode::Yellow:
            if (tick_and_check_timeout())
                set_mode(Mode::Red);
            break;
    }
}

bool TrafficLight::tick_and_check_timeout()
{
    if (m_ticks)
        --m_ticks;

    return m_ticks <= 0;
}


void TrafficLight::set_lights(bool red, bool yellow, bool green)
{
    m_leds[RED].set_status(red);
    m_leds[YELLOW].set_status(yellow);
    m_leds[GREEN].set_status(green);
}
