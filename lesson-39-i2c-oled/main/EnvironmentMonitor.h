#pragma once

#include <bmp280.h>
#include <ds1307.h>
#include <ssd1306.h>
#include <i2cdev.h>
#include <esp_netif.h>
#include <mqtt_client.h>

#include <string>

#include "ClockAdjuster.h"

class EnvironmentMonitor
{
public:
    EnvironmentMonitor();
    ~EnvironmentMonitor();

private:
    enum MessageType {
        LogType = 0,
        EventType = 1,
    };

    struct QueueMessage {
        MessageType type;
        union {
            uint64_t value;
            struct {
                char topic[32];
                char data[32];
            };
            char message[64];
        };
    };

    bmp280_t  m_bmp280 = {};
    i2c_dev_t m_ds1307 = {};
    SSD1306_t m_oled = {};
    TaskHandle_t m_task = {};
    QueueHandle_t m_queue = {};

    esp_netif_t* m_netif = nullptr;
    EventGroupHandle_t m_wifi_event_group = {};
    esp_mqtt_client_handle_t m_mqtt_handle = {};

    volatile bool m_stop_task = false;
    volatile bool m_pause = false;
    volatile bool m_remote_mode = false;

    std::tm m_local_time = {};
    std::tm m_new_time = {};

    std::tm m_mqtt_data_time = {};
    std::string m_mqtt_topic;
    std::string m_mqtt_data;

    ClockAdjuster m_clock_adjuster;
    Button m_mode_switcher;

    void setup_bmp280();
    void setup_ds1307();
    void setup_ssd1306();
    void setup_task();
    void setup_nvs();
    void setup_wifi();

    void mqtt_start();
    void update_task();
    void post_log(const char* message);
    void post_event(const char* topic, size_t topic_len, const char* data, size_t data_len);
    void log_to_screen(int line, const char* fmt, ...);
    bool set_system_time(tm* rtc_time = nullptr);
    bool get_local_time();

    void on_wifi_event(esp_event_base_t event_base, int32_t event_id, void* event_data);
    void on_mqtt_event(esp_event_base_t event_base, int32_t event_id, void* event_data);
    void on_mode_switch();
};
