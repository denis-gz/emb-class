#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/ledc.h>

#include <array>
#include <cmath>

#include "common.h"
#include "sdkconfig.h"

const TickType_t xDelayMs = 20 / portTICK_PERIOD_MS;

const int PWM_FREQ = 1000;
const int PWM_BITS = LEDC_TIMER_10_BIT;

const int FILTER_RESET_THRES = 20;
const int FADE_DURATION_MS = 1000;
const int FADE_DUTY_TARGET = (1 << PWM_BITS) / 2;   // 50%

void setup_adc();
void setup_ledc();
bool ledc_fade_callback(const ledc_cb_param_t* param, void* user_arg);

adc_channel_t g_adc_chan = {};
adc_oneshot_unit_handle_t g_adc_handle = {};
adc_cali_handle_t g_cali_handle = {};

volatile bool g_fade_started = false;

extern "C" void app_main(void)
{
    setup_adc();
    setup_ledc();

    int sum = 0;
    std::array<int, CONFIG_SMA_WINDOW_SIZE> samples = {};

    const float v_range = 1330.0f;
    const float adc_range = 4095.0f;
    size_t i = 0;  // Points to the last added sample

    do {
        int new_sample = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(g_adc_handle, g_adc_chan, &new_sample));

        samples[i % CONFIG_SMA_WINDOW_SIZE] = new_sample;
        int old_sample = samples[++i % CONFIG_SMA_WINDOW_SIZE];
        float count = i < CONFIG_SMA_WINDOW_SIZE ? i : CONFIG_SMA_WINDOW_SIZE;

        // Calculate mean (average)
        sum += new_sample;
        float avg = sum / count;
        sum -= old_sample;

        // Calculate standard deviation
        float sigma = 0;
        if (count > 1) {
            for (int j = 0; j < count; ++j) {
                float s = samples[j] - avg;
                sigma += s * s;
            }
            sigma = std::sqrt(sigma / (count - 1));
        }

        // Detect signal change, reset filter
        if (sigma > FILTER_RESET_THRES && i > 3) {
            avg = new_sample;
            samples.fill(0);
            sum = 0;
            i = 0;
        }

        // Start fading when RP voltage is zero, otherwise set brightness according to voltage level
        if (avg == 0) {
            if (!g_fade_started) {
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 0);
                ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, FADE_DUTY_TARGET, FADE_DURATION_MS, LEDC_FADE_NO_WAIT);
                g_fade_started = true;
            }
        }
        else {
            if (g_fade_started) {
                ledc_fade_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                g_fade_started = false;
            }
            else {
                int duty = avg / adc_range * (1 << PWM_BITS);
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 0);
                //ESP_LOGI(TAG, "Avg: %6.1f | Duty: %4d (%4.1f%%)", avg, duty, duty_r * 100.0f);
            }
        }

        vTaskDelay(xDelayMs);
    }
    while (1);
}

void setup_adc()
{
    adc_unit_t unit_id = {};
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(CONFIG_PIN_ADC_INPUT, &unit_id, &g_adc_chan));
    ESP_LOGI(TAG, "Working with ADC_UNIT_%d, ADC_CHANNEL_%0d", unit_id + 1, g_adc_chan);

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit_id
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &g_adc_handle));

    // RP divider is expected to be in series with 20K resistor on ADC pin
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_2_5,          // 1250 mV range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_adc_handle, g_adc_chan, &config));

    do {
        adc_cali_scheme_ver_t scheme_mask = {};
        if (adc_cali_check_scheme(&scheme_mask) != ESP_OK)
            break;
        if (!(scheme_mask & ADC_CALI_SCHEME_VER_CURVE_FITTING))
            break;
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit_id,
            .atten = config.atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        adc_cali_create_scheme_curve_fitting(&cali_config, &g_cali_handle);
    }
    while (0);

    assert(g_cali_handle);
}

void setup_ledc()
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = static_cast<ledc_timer_bit_t>(PWM_BITS),
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = CONFIG_PIN_LED_1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .flags { .output_invert = 1 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    ledc_cbs_t cbs = { .fade_cb = ledc_fade_callback };
    ESP_ERROR_CHECK(ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, &cbs, nullptr));
}

bool IRAM_ATTR ledc_fade_callback(const ledc_cb_param_t* param, void* user_arg)
{
    if (param->duty == 0) {
        ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, FADE_DUTY_TARGET, FADE_DURATION_MS, LEDC_FADE_NO_WAIT);
    }
    else {
        ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, FADE_DURATION_MS, LEDC_FADE_NO_WAIT);
    }
    return false;
}
