#include "MelodyPlayer.h"

#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include "common.h"

MelodyPlayer::MelodyPlayer(gpio_num_t buzzeк_pin, ledc_timer_t ledc_timer, ledc_channel_t ledc_channel)
    : m_ledc_timer(ledc_timer)
    , m_ledc_channel(ledc_channel)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = m_ledc_timer,
        .freq_hz = 1000,                    // Can't pass zero here for initial config
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .gpio_num = buzzeк_pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = m_ledc_channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = m_ledc_timer,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    m_metronome_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP,      // Counting direction is up
        .resolution_hz = 100 * 1000,        // Resolution is 100 kHz, i.e., 1 tick equals 10 microseconds
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&m_metronome_config, &m_metronome));

    gptimer_event_callbacks_t cbs = { .on_alarm = on_metronome_tick };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(m_metronome, &cbs, this));
    ESP_ERROR_CHECK(gptimer_enable(m_metronome));
}

MelodyPlayer::~MelodyPlayer()
{
    Stop();
    gptimer_disable(m_metronome);
    gptimer_del_timer(m_metronome);

    ledc_timer_config_t deconf { .deconfigure = true };
    ledc_timer_config(&deconf);
}

void MelodyPlayer::Play(const note_t* begin, const note_t* end, uint note_duration)
{
    m_begin = begin;
    m_end = end;
    m_progress = 0;

    // Schedule the timer for note duration
    gptimer_alarm_config_t tick_config = {
        .alarm_count = note_duration * m_metronome_config.resolution_hz / 1000,
        .reload_count = 0,
        .flags { .auto_reload_on_alarm = 1 }
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(m_metronome, &tick_config));
    ESP_ERROR_CHECK(gptimer_set_raw_count(m_metronome, 0));
    ESP_ERROR_CHECK(gptimer_start(m_metronome));

    play_note(begin[m_progress]);
}

void MelodyPlayer::Stop()
{
    gptimer_stop(m_metronome);
    ledc_stop(LEDC_LOW_SPEED_MODE, m_ledc_channel, 0);
}

void MelodyPlayer::play_note(note_t freq)
{
    if (freq) {
        // Play note
        const uint32_t duty = (1 << LEDC_TIMER_8_BIT) / 2;
        ledc_set_freq(LEDC_LOW_SPEED_MODE, m_ledc_timer, freq);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel);
    }
    else {
        // Make a pause
        ledc_set_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, m_ledc_channel);
    }

    ++m_progress;
}

bool IRAM_ATTR MelodyPlayer::on_metronome_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    auto& _ = *static_cast<MelodyPlayer*>(user_ctx);

    if (_.m_progress < std::distance(_.m_begin, _.m_end)) {
        _.play_note(_.m_begin[_.m_progress]);
    }
    else {
        ledc_stop(LEDC_LOW_SPEED_MODE, _.m_ledc_channel, 0);
    }
    return false;
}
