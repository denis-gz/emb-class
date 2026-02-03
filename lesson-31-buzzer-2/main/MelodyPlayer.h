#pragma once

#include <driver/ledc.h>
#include <driver/gptimer.h>
#include <sys/types.h>
#include <iterator>

class MelodyPlayer
{
public:
    typedef uint note_t;

    MelodyPlayer(gpio_num_t buzzer_pin,
                 ledc_timer_t ledc_timer = LEDC_TIMER_0,
                 ledc_channel_t ledc_channel = LEDC_CHANNEL_0);
    ~MelodyPlayer();

    void Play(const note_t* begin, const note_t* end, uint note_duration);
    void Stop();

    uint Progress() const { return m_progress; }
    bool IsPlaying() const { return m_progress < std::distance(m_begin, m_end); }

private:
    const ledc_timer_t m_ledc_timer;
    const ledc_channel_t m_ledc_channel;
    gptimer_handle_t m_metronome = nullptr;
    gptimer_config_t m_metronome_config = {};
    gptimer_alarm_config_t m_alarm_config = {};

    uint m_progress = 0;
    const note_t* m_begin = nullptr;
    const note_t* m_end = nullptr;

    void play_note(note_t note);

    static bool on_metronome_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);
};
