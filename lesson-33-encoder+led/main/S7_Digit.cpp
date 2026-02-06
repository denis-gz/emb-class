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
    gpio_output_set(0, m_config.bit_mask(), 0, 0);
    gpio_set_level(static_cast<gpio_num_t>(m_config.pin_COM), SET_OFF);
}

void S7_Digit::Refresh()
{
    gpio_set_level(static_cast<gpio_num_t>(m_config.pin_COM), SET_OFF);

    if (m_segments) {
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_A),   set_active(m_segments & SEG_A));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_B),   set_active(m_segments & SEG_B));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_C),   set_active(m_segments & SEG_C));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_D),   set_active(m_segments & SEG_D));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_E),   set_active(m_segments & SEG_E));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_F),   set_active(m_segments & SEG_F));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_G),   set_active(m_segments & SEG_G));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_DP),  set_active(m_segments & SEG_DP));
        gpio_set_level(static_cast<gpio_num_t>(m_config.pin_COM), SET_ON);
    }
}
