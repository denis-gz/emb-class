#include "EnvironmentMonitor.h"

#include <freertos/queue.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <mqtt_client.h>

#include <cstring>
#include <cmath>

#include "common.h"
#include "wifi_creds.h"
#include "mqtt_creds.h"

#define WIFI_CONNECTED_BIT BIT0
#define MQTT_STARTED_BIT   BIT1
#define MQTT_CONNECTED_BIT BIT2

EnvironmentMonitor::EnvironmentMonitor()
    : m_clock_adjuster(
        [this] (tm* info) { *info = m_local_time; },     // Get time callback
        [this] (tm* info) { m_new_time = *info; })       // Set time callback
    , m_mode_switcher(CONFIG_PIN_DEVKIT_BUTTON, [this] { on_mode_switch(); })
{
    ESP_LOGI(TAG, "Running on core #%d", xPortGetCoreID());
    m_queue = xQueueCreate(10, sizeof(QueueMessage));
    assert(m_queue);

    ESP_ERROR_CHECK(i2cdev_init());

    setup_bmp280();
    setup_ds1307();
    setup_ssd1306();
    setup_nvs();
    setup_wifi();
    setup_task();
}

EnvironmentMonitor::~EnvironmentMonitor()
{
    m_stop_task = true;

    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(m_netif);
    esp_netif_deinit();
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

    tm rtc_time;
    ESP_ERROR_CHECK(ds1307_get_time(&m_ds1307, &rtc_time));
    set_system_time(&rtc_time);
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
    xTaskCreatePinnedToCore(
        member_cast<TaskFunction_t>(&EnvironmentMonitor::update_task),
        "update_task",          // A descriptive name for debugging
        4096,                   // Stack size (4096 bytes is very safe for I2C and OLED strings)
        this,                   // Parameter passed to the task (pointer to object)
        5,                      // Task priority (0 is lowest, configMAX_PRIORITIES-1 is highest)
        nullptr,                // No task handle needed
        xPortGetCoreID()        // Pin to the same core
    );
}

void EnvironmentMonitor::update_task()
{
    const TickType_t xTimeout = pdMS_TO_TICKS(100);

    const char PROGRESS[] = "-\\|/";
    int p_index = 0;

    uint64_t counter = 0;

    typedef char line_t[17];

    while (!m_stop_task) {
        if (m_new_time.tm_year) {
            m_local_time = m_new_time;
            set_system_time();
            memset(&m_new_time, 0, sizeof(m_new_time));
        }

        if (get_local_time()) {
            line_t line_0 = {};
            strftime(line_0, sizeof(line_0), "%H:%M:%S", &m_local_time);
            ssd1306_display_text(&m_oled, 0, line_0, 16, false);

            line_t line_1 = {};
            strftime(line_1, sizeof(line_1), "%a %d.%m.%Y", &m_local_time);
            ssd1306_display_text(&m_oled, 1, line_1, 16, false);
        }

        line_t lines[3] = {};

        QueueMessage qmsg;
        if (xQueueReceive(m_queue, &qmsg, xTimeout)) {
            switch (qmsg.type) {
                case LogType:
                    log_to_screen(7, qmsg.message);
                    break;
                case EventType:
                    if (m_remote_mode) {
                        // Display remote data from MQTT
                        if (std::strstr(qmsg.topic, "/temp")) {
                            snprintf(lines[0], sizeof(lines[0]), "/temp :%9s", qmsg.data);
                            ssd1306_display_text(&m_oled, 3, lines[0], 16, false);
                        }
                        if (std::strstr(qmsg.topic, "/pres")) {
                            snprintf(lines[1], sizeof(lines[1]), "/pres :%9s", qmsg.data);
                            ssd1306_display_text(&m_oled, 4, lines[1], 16, false);
                        }
                        if (std::strstr(qmsg.topic, "/hum")) {
                            snprintf(lines[2], sizeof(lines[2]), "/hum  :%9s", qmsg.data);
                            ssd1306_display_text(&m_oled, 5, lines[2], 16, false);
                        }
                        log_to_screen(7, qmsg.topic);
                    }
                    else {
                        // Otherwise just show some activity from MQTT
                        log_to_screen(7, "MQTT [%c]", PROGRESS[p_index++ % 4]);
                    }
                    break;
                default:
                    break;
            }
        }

        ssd1306_clear_line(&m_oled, 2, false);

        if (++counter % 10 != 0)
            continue;

        auto uxBits = xEventGroupWaitBits(m_wifi_event_group, WIFI_CONNECTED_BIT | MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, 0);
        if (uxBits & WIFI_CONNECTED_BIT) {
            float temp = NAN;
            float pres = NAN;
            float humi = NAN;

            if (bmp280_read_float(&m_bmp280, &temp, &pres, &humi) == ESP_OK) {
                if (!m_remote_mode) {
                    snprintf(lines[0], sizeof(lines[0]), "T: %.1f C", temp);
                    ssd1306_display_text(&m_oled, 3, lines[0], 16, false);

                    snprintf(lines[1], sizeof(lines[1]), "P: %4u hPa", (uint) (pres / 100.f));
                    ssd1306_display_text(&m_oled, 4, lines[1], 16, false);

                    snprintf(lines[2], sizeof(lines[2]), "H: %.1f %%", humi);
                    ssd1306_display_text(&m_oled, 5, lines[2], 16, false);
                }
            }
            else {
                if (!m_remote_mode) {
                    ssd1306_clear_line(&m_oled, 3, false);
                    ssd1306_clear_line(&m_oled, 4, false);
                    ssd1306_clear_line(&m_oled, 5, false);
                }
                log_to_screen(7, "BME280 error");
            }

            if (uxBits & MQTT_CONNECTED_BIT) {
                if (!std::isnan(temp))
                    esp_mqtt_client_publish(m_mqtt_handle, MQTT_PUB_TOPIC "/temp", lines[0], 0, 0, 0);
                if (!std::isnan(pres))
                    esp_mqtt_client_publish(m_mqtt_handle, MQTT_PUB_TOPIC "/pres", lines[1], 0, 0, 0);
                if (!std::isnan(humi))
                    esp_mqtt_client_publish(m_mqtt_handle, MQTT_PUB_TOPIC "/humi", lines[2], 0, 0, 0);
            }
        }
    }

    i2c_master_bus_rm_device(m_oled._i2c_dev_handle);
    ds1307_free_desc(&m_ds1307);
    bmp280_free_desc(&m_bmp280);

    i2cdev_done();

    vTaskDelete(nullptr);
}

void EnvironmentMonitor::post_log(const char* message)
{
    QueueMessage qmsg { .type = LogType };
    strncpy(qmsg.message, message, sizeof(qmsg.message))[sizeof(qmsg.message) - 1] = '\0';

    if (xQueueSend(m_queue, &qmsg, 0) != pdPASS) {
        ESP_LOGI(TAG, "Warning: Display queue is full, dropping MQTT message!\n");
    }
}

void EnvironmentMonitor::post_event(const char* topic, size_t topic_len, const char* data, size_t data_len)
{
    QueueMessage qmsg { .type = EventType };
    strncpy(qmsg.topic, topic, std::min(topic_len, sizeof(qmsg.topic) - 1));
    strncpy(qmsg.data, data, std::min(data_len, sizeof(qmsg.data) - 1));

    if (xQueueSend(m_queue, &qmsg, 0) != pdPASS) {
        ESP_LOGI(TAG, "Warning: Display queue is full, dropping MQTT message!\n");
    }
}

void EnvironmentMonitor::log_to_screen(int line, const char* fmt, ...)
{
    char buf[256];
    memset(buf, 0x20, sizeof(buf));

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    const size_t len = strlen(buf);
    buf[len] = 0x20;

    for (int i = 0; i + line < 8; ++i) {
        ssd1306_display_text(&m_oled, i + line, buf + i * 16, 16, false);
    }

    ESP_LOGI(TAG, "%s", buf);
}

bool EnvironmentMonitor::set_system_time(tm* rtc_time)
{
    char* current_tz = nullptr;
    time_t utc_ts = 0;

    if (rtc_time) {
        current_tz = getenv("TZ");
        setenv("TZ", "GMT0", 1);
        tzset();
        utc_ts = std::mktime(rtc_time);
    }
    else {
        if (m_local_time.tm_year > 1900)
            m_local_time.tm_year -= 1900;
        m_local_time.tm_isdst = -1;
        utc_ts = std::mktime(&m_local_time);
    }
    timeval tv = { .tv_sec = utc_ts };
    settimeofday(&tv, nullptr);

    if (rtc_time) {
        // Restore actual TZ value
        if (current_tz) {
            setenv("TZ", current_tz, 1);
            tzset();
        }
        return true;
    }
    else {
        // Always maintain system time in UTC timezone
        if (tm* utc_tm = std::gmtime(&utc_ts)) {
            ds1307_set_time(&m_ds1307, utc_tm);
            return true;
        }
        return false;
    }
}

bool EnvironmentMonitor::get_local_time()
{
    time_t utc_now = std::time(nullptr);
    if (tm* local = std::localtime(&utc_now)) {
        m_local_time = *local;
        return true;
    }
    return false;
}

void EnvironmentMonitor::on_wifi_event(esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(m_wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
        log_to_screen(7, "Connecting Wi-Fi");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(m_wifi_event_group, WIFI_CONNECTED_BIT);
        log_to_screen(7, "Wi-Fi connected");

        auto uxBits = xEventGroupWaitBits(m_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 0);
        if (!(uxBits & MQTT_STARTED_BIT)) {
            mqtt_start();
        }
    }
}

void EnvironmentMonitor::setup_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    else {
        ESP_ERROR_CHECK(ret);
    }
}

void EnvironmentMonitor::setup_wifi()
{
    m_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    m_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        member_cast<esp_event_handler_t>(&EnvironmentMonitor::on_wifi_event), this
    ));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        member_cast<esp_event_handler_t>(&EnvironmentMonitor::on_wifi_event), this
    ));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    log_to_screen(7, "Starting Wi-Fi");
}

void EnvironmentMonitor::mqtt_start()
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;

    m_mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(m_mqtt_handle, MQTT_EVENT_ANY, member_cast<esp_event_handler_t>(&EnvironmentMonitor::on_mqtt_event), this));
    ESP_ERROR_CHECK(esp_mqtt_client_start(m_mqtt_handle));

    xEventGroupSetBits(m_wifi_event_group, MQTT_STARTED_BIT);
}


void EnvironmentMonitor::on_mqtt_event(esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            post_log("MQTT connected");
            esp_mqtt_client_subscribe(m_mqtt_handle, MQTT_SUB_TOPIC, 0);
            xEventGroupSetBits(m_wifi_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            post_log("MQTT disconnect");
            xEventGroupClearBits(m_wifi_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DATA:
            post_event(event->topic, event->topic_len, event->data, event->data_len);
            break;
        default:
            break;
    }
}

void EnvironmentMonitor::on_mode_switch()
{
    m_remote_mode = !m_remote_mode;
}
