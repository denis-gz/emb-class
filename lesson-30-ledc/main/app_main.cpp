#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
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

// ADC filter setup
const int FILTER_RESET_THRES = 20;

// LED setup
const int PWM_LED_FREQ = 1000;
const int PWM_LED_BITS = LEDC_TIMER_10_BIT;
const int PWM_LED_RANGE = (1 << PWM_LED_BITS);
const ledc_channel_t PWM_LED_CHAN = LEDC_CHANNEL_0;
const ledc_timer_t PWM_LED_TIMER = LEDC_TIMER_0;
const float LED_ADC_RANGE = 3660.0f;

const int FADE_DURATION_MS = 1000;
const int FADE_DUTY_TARGET = PWM_LED_RANGE / 2;   // 50%

// SERVO setup
const int PWM_SERVO_FREQ = 50;
const int PWM_SERVO_BITS = LEDC_TIMER_12_BIT;
const int PWM_SERVO_RANGE = (1 << PWM_SERVO_BITS);
const ledc_channel_t PWM_SERVO_CHAN = LEDC_CHANNEL_1;
const ledc_timer_t PWM_SERVO_TIMER = LEDC_TIMER_1;
const float SERVO_ADC_RANGE = 2900.0f;
const float SERVO_ADC_NEG_LIMIT = 630.0f;
const float SERVO_ADC_POS_LIMIT = 2330.0f;

const int SERVO_DUTY_NEG_LIMIT = 7 * PWM_SERVO_RANGE * PWM_SERVO_FREQ / 10000;
const int SERVO_DUTY_POS_LIMIT = 24 * PWM_SERVO_RANGE * PWM_SERVO_FREQ / 10000;

void setup_led_adc();
void setup_led_pwm();
void process_led_input();

void setup_servo_adc();
void setup_servo_pwm();
void process_servo_input();

adc_channel_t g_led_adc_chan = {};
adc_oneshot_unit_handle_t g_led_adc_handle = {};

adc_channel_t g_servo_adc_chan = {};
adc_oneshot_unit_handle_t g_servo_adc_handle = {};

int g_log_counter = 0;

extern "C" void app_main(void)
{
    setup_led_adc();
    setup_led_pwm();

    setup_servo_adc();
    setup_servo_pwm();

    do {
        ++g_log_counter;

        process_led_input();
        process_servo_input();

        vTaskDelay(xDelayMs);
    }
    while (1);
}

float get_filtered_led_adc_sample()
{
    static int s_sum = 0;
    static std::array<int, CONFIG_SMA_WINDOW_SIZE> s_samples = {};

    static size_t s_i = 0;  // Points to the last added sample

    int new_sample = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(g_led_adc_handle, g_led_adc_chan, &new_sample));

    s_samples[s_i % CONFIG_SMA_WINDOW_SIZE] = new_sample;
    int old_sample = s_samples[++s_i % CONFIG_SMA_WINDOW_SIZE];
    float count = s_i < CONFIG_SMA_WINDOW_SIZE ? s_i : CONFIG_SMA_WINDOW_SIZE;

    // Calculate mean (average)
    s_sum += new_sample;
    float avg = s_sum / count;
    s_sum -= old_sample;

    // Calculate standard deviation
    float sigma = 0;
    if (count > 1) {
        for (int j = 0; j < count; ++j) {
            float s = s_samples[j] - avg;
            sigma += s * s;
        }
        sigma = std::sqrt(sigma / (count - 1));
    }

    // Detect signal change, reset filter
    if (sigma > FILTER_RESET_THRES && s_i > 3) {
        avg = new_sample;
        s_samples.fill(0);
        s_sum = 0;
        s_i = 0;
    }

    return avg;
}

bool IRAM_ATTR ledc_fade_callback(const ledc_cb_param_t* param, void* user_arg)
{
    if (param->duty == 0) {
        ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, PWM_LED_CHAN, FADE_DUTY_TARGET, FADE_DURATION_MS, LEDC_FADE_NO_WAIT);
    }
    else {
        ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, PWM_LED_CHAN, 0, FADE_DURATION_MS, LEDC_FADE_NO_WAIT);
    }
    return false;
}

void process_led_input()
{
    static bool s_fade_started = false;

    float led_sample = get_filtered_led_adc_sample();
    if (led_sample < 10)
        led_sample = 0;

    // Start fading when RP voltage is zero, otherwise set brightness according to voltage level
    if (led_sample == 0) {
        if (!s_fade_started) {
            ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, PWM_LED_CHAN, 0, 0);
            ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, PWM_LED_CHAN, FADE_DUTY_TARGET, FADE_DURATION_MS, LEDC_FADE_NO_WAIT);
            s_fade_started = true;
        }
    }
    else {
        if (s_fade_started) {
            ledc_fade_stop(LEDC_LOW_SPEED_MODE, PWM_LED_CHAN);
            s_fade_started = false;
        }
        else {
            float duty_r = led_sample / LED_ADC_RANGE;
            int duty = duty_r * PWM_LED_RANGE;
            ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, PWM_LED_CHAN, duty, 0);
            if (g_log_counter % 50 == 0) {
                ESP_LOGI(TAG, "LED >>> Avg: %6.1f | Duty: %4d (%4.1f%%)", led_sample, duty, duty_r * 100.0f);
            }
        }
    }
}

void setup_led_adc()
{
    adc_unit_t unit_id = {};
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(CONFIG_PIN_ADC_INPUT_1, &unit_id, &g_led_adc_chan));
    ESP_LOGI(TAG, "LED >>> Working with ADC_UNIT_%d, ADC_CHANNEL_%0d", unit_id + 1, g_led_adc_chan);

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit_id
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &g_led_adc_handle));

    // RP divider is expected to be in series with 20K resistor on ADC pin
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_2_5,          // 1250 mV range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_led_adc_handle, g_led_adc_chan, &config));
}

void setup_led_pwm()
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = static_cast<ledc_timer_bit_t>(PWM_LED_BITS),
        .timer_num = PWM_LED_TIMER,
        .freq_hz = PWM_LED_FREQ,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = CONFIG_PIN_LED_1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_LED_CHAN,
        .timer_sel = PWM_LED_TIMER,
        .flags { .output_invert = 1 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    ledc_cbs_t cbs = { .fade_cb = ledc_fade_callback };
    ESP_ERROR_CHECK(ledc_cb_register(LEDC_LOW_SPEED_MODE, PWM_LED_CHAN, &cbs, nullptr));
}

float get_filtered_servo_adc_sample()
{
    static int s_sum = 0;
    static std::array<int, CONFIG_SMA_WINDOW_SIZE> s_samples = {};

    static size_t s_i = 0;  // Points to the last added sample

    int new_sample = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(g_servo_adc_handle, g_servo_adc_chan, &new_sample));

    s_samples[s_i % CONFIG_SMA_WINDOW_SIZE] = new_sample;
    int old_sample = s_samples[++s_i % CONFIG_SMA_WINDOW_SIZE];
    float count = s_i < CONFIG_SMA_WINDOW_SIZE ? s_i : CONFIG_SMA_WINDOW_SIZE;

    // Calculate mean (average)
    s_sum += new_sample;
    float avg = s_sum / count;
    s_sum -= old_sample;

    // Calculate standard deviation
    float sigma = 0;
    if (count > 1) {
        for (int j = 0; j < count; ++j) {
            float s = s_samples[j] - avg;
            sigma += s * s;
        }
        sigma = std::sqrt(sigma / (count - 1));
    }

    // Detect signal change, reset filter
    if (sigma > FILTER_RESET_THRES && s_i > 3) {
        avg = new_sample;
        s_samples.fill(0);
        s_sum = 0;
        s_i = 0;
    }

    return avg;
}

void process_servo_input()
{
    float servo_sample = get_filtered_servo_adc_sample();
    if (servo_sample < SERVO_ADC_NEG_LIMIT)
        servo_sample = SERVO_ADC_NEG_LIMIT;
    if (servo_sample > SERVO_ADC_POS_LIMIT)
        servo_sample = SERVO_ADC_POS_LIMIT;

    float duty_r = (servo_sample - SERVO_ADC_NEG_LIMIT) / (SERVO_ADC_POS_LIMIT - SERVO_ADC_NEG_LIMIT);
    int duty = SERVO_DUTY_NEG_LIMIT + duty_r * (SERVO_DUTY_POS_LIMIT - SERVO_DUTY_NEG_LIMIT);

    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, PWM_SERVO_CHAN, duty, 0);
    if (g_log_counter % 50 == 0) {
        float angle = -90.0f + duty_r * 180.0f;
        ESP_LOGI(TAG, "SRV >>> Avg: %6.1f | Duty: %4d (%4.1f%%) | Angle: %+6.1f", servo_sample, duty, duty_r * 100.0f, angle);
    }
}

void setup_servo_pwm()
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = static_cast<ledc_timer_bit_t>(PWM_SERVO_BITS),
        .timer_num = PWM_SERVO_TIMER,
        .freq_hz = PWM_SERVO_FREQ,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = CONFIG_PIN_SERVO_1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = PWM_SERVO_CHAN,
        .timer_sel = PWM_SERVO_TIMER,
        .duty = (SERVO_DUTY_NEG_LIMIT + SERVO_DUTY_POS_LIMIT) / 2,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void setup_servo_adc()
{
    adc_unit_t unit_id = {};
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(CONFIG_PIN_ADC_INPUT_2, &unit_id, &g_servo_adc_chan));
    ESP_LOGI(TAG, "SRV >>> Working with ADC_UNIT_%d, ADC_CHANNEL_%0d", unit_id + 1, g_servo_adc_chan);

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit_id
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &g_servo_adc_handle));

    // RP divider is expected to be in series with 100K resistor on ADC pin
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_2_5,          // 1250 mV range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_servo_adc_handle, g_servo_adc_chan, &config));
}
