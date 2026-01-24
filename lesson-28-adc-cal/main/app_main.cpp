#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

#include <cmath>

#include "common.h"
#include "sdkconfig.h"

const TickType_t xDelayMs = 100 / portTICK_PERIOD_MS;


extern "C" void app_main(void)
{
    adc_unit_t unit_id = {};
    adc_channel_t channel_id = {};
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(CONFIG_PIN_ADC_INPUT, &unit_id, &channel_id));
    ESP_LOGI(TAG, "Working with ADC_UNIT_%d, ADC_CHANNEL_%0d", unit_id + 1, channel_id);

    adc_oneshot_unit_handle_t adc_handle = {};
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unit_id
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // RP divider is expected to be in series with 20K resistor on ADC pin
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_2_5,          // 1250 mV range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel_id, &config));

    adc_cali_handle_t cal_handle = {};
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
        adc_cali_create_scheme_curve_fitting(&cali_config, &cal_handle);
    }
    while (0);

    int sum = 0;
    int samples[CONFIG_SMA_WINDOW_SIZE] = {};

    const float v_range = 1250.0f;
    const float adc_range = 4095.0f;
    size_t i = 0;  // Points to the last added sample

    do {
        int new_sample = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel_id, &new_sample));

        samples[i % CONFIG_SMA_WINDOW_SIZE] = new_sample;
        int old_sample = samples[++i % CONFIG_SMA_WINDOW_SIZE];
        float count = i < CONFIG_SMA_WINDOW_SIZE ? i : CONFIG_SMA_WINDOW_SIZE;

        // Calculate mean (average)
        sum += new_sample;
        float mean = sum / count;
        sum -= old_sample;

        // Calculate standard deviation
        float sigma = 0;
        for (int j = 0; j < count; ++j) {
            float s = samples[j] - mean;
            sigma += s * s;
        }
        sigma = std::sqrt(sigma / (count - 1));

        int expected_voltage = mean / adc_range * v_range;
        int actual_voltage = 0;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cal_handle, (int) std::round(mean), &actual_voltage));

        float error = 100.0f * (expected_voltage - actual_voltage) / actual_voltage;
        ESP_LOGI(TAG, "Raw: %4d | Avg: %6.1f | Sig: %8.4f | Exp: %4d mV | Act: %4d mV | Err: %+4.1f%%",
            new_sample, mean, sigma, expected_voltage, actual_voltage, error);

        vTaskDelay(xDelayMs);
    }
    while (1);
}

