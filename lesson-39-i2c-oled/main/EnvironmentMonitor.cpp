#include "EnvironmentMonitor.h"

#include <esp_log.h>
#include <cstring>

#include "common.h"

EnvironmentMonitor::EnvironmentMonitor()
    : m_clock_adjuster(
        [this] (tm* info) { *info = m_time_info; },     // Get time callback
        [this] (tm* info) { m_new_time = *info; })      // Set time callback
{
    ESP_ERROR_CHECK(i2cdev_init());

    setup_bmp280();
    setup_ds1307();
    setup_ssd1306();
    setup_task();
}

EnvironmentMonitor::~EnvironmentMonitor()
{
    m_stop_task = true;
}

void EnvironmentMonitor::setup_bmp280()
{
    bmp280_params_t params = {};
    ESP_ERROR_CHECK(bmp280_init_default_params(&params));

    ESP_ERROR_CHECK(bmp280_init_desc(&m_bmp280,
        CONFIG_I2C_BMP280_ADDR, I2C_NUM_0,
        static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SDA_PIN),
        static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SCL_PIN)));

    ESP_ERROR_CHECK(bmp280_init(&m_bmp280, &params));
}

void EnvironmentMonitor::setup_ds1307()
{
    ESP_ERROR_CHECK(ds1307_init_desc(&m_ds1307, I2C_NUM_0,
        static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SDA_PIN),
        static_cast<gpio_num_t>(CONFIG_I2CDEV_DEFAULT_SCL_PIN)));

    ESP_ERROR_CHECK(ds1307_get_time(&m_ds1307, &m_time_info));
}

void EnvironmentMonitor::setup_ssd1306()
{
    i2cdev_get_shared_handle(I2C_NUM_0, reinterpret_cast<void**>(&m_oled._i2c_bus_handle));
    i2c_device_add(&m_oled, I2C_NUM_0, GPIO_NUM_NC, CONFIG_I2C_SSD1306_ADDR);

    #ifdef CONFIG_SSD1306_128x64
    ssd1306_init(&m_oled, 128, 64);
    #elif CONFIG_SSD1306_128x32
    ssd1306_init(&m_oled, 128, 32);
    #endif

    ssd1306_clear_screen(&m_oled, false);
    ssd1306_contrast(&m_oled, 0xFF);
}

void EnvironmentMonitor::setup_task()
{
    union {
        void (EnvironmentMonitor::*member)();
        TaskFunction_t task;
    }
    handler = {
        .member = &EnvironmentMonitor::update_task
    };

    xTaskCreate(
        handler.task,           // The function that implements the task
        "update_task",          // A descriptive name for debugging
        4096,                   // Stack size (4096 bytes is very safe for I2C and OLED strings)
        this,                   // Parameter passed to the task (pointer to object)
        5,                      // Task priority (0 is lowest, configMAX_PRIORITIES-1 is highest)
        nullptr
    );
}

void EnvironmentMonitor::update_task()
{
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (!m_stop_task) {
        if (m_new_time.tm_year) {
            ds1307_set_time(&m_ds1307, &m_new_time);
            memset(&m_new_time, 0, sizeof(m_new_time));
        }
        if (ds1307_get_time(&m_ds1307, &m_time_info) == ESP_OK) {
            char line_0[16] = {};
            strftime(line_0, sizeof(line_0), "%H:%M:%S", &m_time_info);
            ssd1306_display_text(&m_oled, 0, line_0, 14, false);

            char line_1[16] = {};
            strftime(line_1, sizeof(line_1), "%a %d.%m.%Y", &m_time_info);
            ssd1306_display_text(&m_oled, 1, line_1, 14, false);
        }

        ssd1306_clear_line(&m_oled, 2, false);

        float temperature = 0;
        float pressure = 0;
        float humidity = 0;

        if (bmp280_read_float(&m_bmp280, &temperature, &pressure, &humidity) == ESP_OK) {
            char line_3[16] = {};
            snprintf(line_3, sizeof(line_3), "T: %.1f C", temperature);
            ssd1306_display_text(&m_oled, 3, line_3, 14, false);

            char line_4[16] = {};
            snprintf(line_4, sizeof(line_4), "P: %u hPa", (uint) (pressure / 100.f));
            ssd1306_display_text(&m_oled, 4, line_4, 14, false);

            char line_5[16] = {};
            snprintf(line_5, sizeof(line_5), "H: %.1f %%", humidity);
            ssd1306_display_text(&m_oled, 5, line_5, 14, false);
        }
        else {
            ssd1306_display_text(&m_oled, 3, "BME280 error", 14, false);
            ssd1306_clear_line(&m_oled, 4, false);
            ssd1306_clear_line(&m_oled, 5, false);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }

    i2c_master_bus_rm_device(m_oled._i2c_dev_handle);
    ds1307_free_desc(&m_ds1307);
    bmp280_free_desc(&m_bmp280);

    i2cdev_done();

    vTaskDelete(nullptr);
}
