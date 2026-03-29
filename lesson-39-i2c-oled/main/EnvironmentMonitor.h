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
    bmp280_t  m_bmp280 = {};
    i2c_dev_t m_ds1307 = {};
    SSD1306_t m_oled = {};
    TaskHandle_t m_task = {};

    volatile bool m_stop_task = false;

    std::tm m_local_time = {};
    std::tm m_new_time = {};

    ClockAdjuster m_clock_adjuster;

    void setup_bmp280();
    void setup_ds1307();
    void setup_ssd1306();
    void setup_task();

    void update_task();
    bool set_system_time(tm* rtc_time = nullptr);
    bool get_local_time();
};
