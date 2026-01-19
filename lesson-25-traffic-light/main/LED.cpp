#include "LED.hpp"

LED::LED()
{ }

LED::~LED()
{
    if (m_pin != GPIO_NUM_NC) {
        gpio_reset_pin(m_pin);
    }
}

esp_err_t LED::init(gpio_num_t pin)
{
    m_pin = pin;

    gpio_config_t gpio_conf = {
        .pin_bit_mask = 1ULL << pin,
        .mode         = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    esp_err_t ret;
    ESP_ERROR_CHECK(ret = gpio_config(&gpio_conf));
    ESP_ERROR_CHECK(ret = gpio_set_level(m_pin, 1));
    return ret;
}

void LED::set_status(bool status)
{
    gpio_set_level(m_pin, status ? 0 : 1);
    m_status = status;
}

bool LED::get_status() const
{
    return m_status;
}

