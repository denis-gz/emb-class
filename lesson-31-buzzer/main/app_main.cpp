#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2s_pdm.h"

#define SAMPLE_RATE     44100
#define AMPLITUDE       2000*16        // safe for PDM + RC filter

#define NOTE_E7  2637
#define NOTE_C7  2093
#define NOTE_G7  3136
#define NOTE_G6  1568
#define NOTE_E6  1319
#define NOTE_A6  1760
#define NOTE_B6  1976
#define NOTE_AS6 1865
#define NOTE_F7  2794
#define NOTE_D7  2349
#define NOTE_A7  3520

typedef struct {
    int freq;
    int duration_ms;
} note_t;

// Super Mario Bros main theme (intro)
static const note_t melody[] = {
    {NOTE_E7, 150}, {NOTE_E7, 150}, {0, 150},
    {NOTE_E7, 150}, {0, 100},
    {NOTE_C7, 150}, {NOTE_E7, 150}, {0, 150},
    {NOTE_G7, 300}, {0, 300},
    {NOTE_G6, 300}, {0, 300},

    {NOTE_C7, 150}, {0, 150},
    {NOTE_G6, 150}, {0, 150},
    {NOTE_E6, 150}, {0, 150},
    {NOTE_A6, 150}, {NOTE_B6, 150},
    {NOTE_AS6, 150}, {NOTE_A6, 150},
    {NOTE_G6, 200}, {NOTE_E7, 200},
    {NOTE_G7, 200}, {NOTE_A7, 150},
    {NOTE_F7, 150}, {NOTE_G7, 150},
    {0, 150},
    {NOTE_E7, 150}, {NOTE_C7, 150},
    {NOTE_D7, 150}, {NOTE_B6, 300}
};


static i2s_chan_handle_t tx_chan;

static void pdm_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        I2S_NUM_0,
        I2S_ROLE_MASTER
    );

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_pdm_tx_config_t pdm_cfg = {
        .clk_cfg  = I2S_PDM_TX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk  = GPIO_NUM_26,
            .dout = static_cast<gpio_num_t>(CONFIG_PIN_BUZZER),
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(
        i2s_channel_init_pdm_tx_mode(tx_chan, &pdm_cfg)
    );
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}


static void play_square(int freq, int duration_ms)
{
    int samples = SAMPLE_RATE * duration_ms / 1000;
    int16_t buffer[256];
    size_t bytes_written;

    float phase = 0.0f;
    float step  = 2.0f * M_PI * freq / SAMPLE_RATE;

    while (samples > 0) {
        int count = samples > 256 ? 256 : samples;

        for (int i = 0; i < count; i++) {
            buffer[i] = (phase < M_PI) ? AMPLITUDE : -AMPLITUDE;
            phase += step;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }

        ESP_ERROR_CHECK(
            i2s_channel_write(
                tx_chan,
                buffer,
                count * sizeof(int16_t),
                &bytes_written,
                portMAX_DELAY
            )
        );

        samples -= count;
    }
}


void app_main(void)
{
    pdm_init();

    while (1) {
        for (int i = 0;
             i < sizeof(melody) / sizeof(note_t);
             i++) {

            play_square(
                melody[i].freq,
                melody[i].duration_ms
            );

            vTaskDelay(pdMS_TO_TICKS(40));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
