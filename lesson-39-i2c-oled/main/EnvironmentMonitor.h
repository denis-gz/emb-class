#pragma once

#include <bmp280.h>
#include <ds1307.h>
#include <ssd1306.h>
#include <i2cdev.h>

#include "ClockAdjuster.h"

class EnvironmentMonitor
{
public:
    EnvironmentMonitor();
    ~EnvironmentMonitor();

private:
    static bool on_alarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);

    bmp280_t  m_bmp280 = {};
    i2c_dev_t m_ds1307 = {};
    SSD1306_t m_oled = {};
    TaskHandle_t m_task = nullptr;

    std::tm m_time_info = {};
    std::tm m_new_time = {};

    ClockAdjuster m_clock_adjuster;

    void setup_bmp280();
    void setup_ds1307();
    void setup_ssd1306();
    void setup_task();

    void update_task();
};
