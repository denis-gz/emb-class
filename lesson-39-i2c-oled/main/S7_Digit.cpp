#include "S7_Digit.h"

#include <driver/gpio.h>
#include <rom/gpio.h>

#define SET_ON  set_active(0)
#define SET_OFF set_active(1)

S7_Digit::S7_Digit()
{ }

void S7_Digit::SetConfig(digit_config_t config)
{
    m_config = config;
}

void S7_Digit::SetSegments(uint8_t segments)
{
    m_segments = segments;
}

void S7_Digit::Hide()
{
    if (m_config.active_level) {
        gpio_output_set(1 << m_config.pin_COM, m_config.bit_mask(), 0, 0);
    }
    else {
        gpio_output_set(m_config.bit_mask(), 1 << m_config.pin_COM, 0, 0);
    }
}

void S7_Digit::Refresh()
{
    uint32_t set_mask = 0;
    uint32_t clear_mask = 0;

    if (m_segments) {
        if (m_config.active_level) {
            // If active level is HIGH, set visible segments pin to 1, invisible to 0
            set_mask = 0
                | (m_segments & SEG_A  ? 1 << m_config.pin_A  : 0)
                | (m_segments & SEG_B  ? 1 << m_config.pin_B  : 0)
                | (m_segments & SEG_C  ? 1 << m_config.pin_C  : 0)
                | (m_segments & SEG_D  ? 1 << m_config.pin_D  : 0)
                | (m_segments & SEG_E  ? 1 << m_config.pin_E  : 0)
                | (m_segments & SEG_F  ? 1 << m_config.pin_F  : 0)
                | (m_segments & SEG_G  ? 1 << m_config.pin_G  : 0)
                | (m_segments & SEG_DP ? 1 << m_config.pin_DP : 0);
            clear_mask = 1 << m_config.pin_COM | (~set_mask & m_config.bit_mask());
        }
        else {
            // If active level is LOW, set visible segments pin to 0, invisible to 1
            clear_mask = 0
                | (m_segments & SEG_A  ? 1 << m_config.pin_A  : 0)
                | (m_segments & SEG_B  ? 1 << m_config.pin_B  : 0)
                | (m_segments & SEG_C  ? 1 << m_config.pin_C  : 0)
                | (m_segments & SEG_D  ? 1 << m_config.pin_D  : 0)
                | (m_segments & SEG_E  ? 1 << m_config.pin_E  : 0)
                | (m_segments & SEG_F  ? 1 << m_config.pin_F  : 0)
                | (m_segments & SEG_G  ? 1 << m_config.pin_G  : 0)
                | (m_segments & SEG_DP ? 1 << m_config.pin_DP : 0);
            set_mask = 1 << m_config.pin_COM | (~clear_mask & m_config.bit_mask());
        }
    }
    else {
        if (m_config.active_level) {
            set_mask = 1 << m_config.pin_COM;
        }
        else {
            clear_mask = 1 << m_config.pin_COM;
        }
    }

    gpio_output_set(set_mask, clear_mask, 0, 0);
}
