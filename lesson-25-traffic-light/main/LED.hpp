#pragma once

#include <driver/gpio.h>

class LED
{
public:
    LED();
    ~LED();

    esp_err_t init(gpio_num_t pin);

    void set_status(bool status);
    bool get_status() const;

private:
    gpio_num_t m_pin = GPIO_NUM_NC;
    bool m_status = false;
};

