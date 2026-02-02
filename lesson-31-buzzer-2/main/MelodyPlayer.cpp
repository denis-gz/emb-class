#include "MelodyPlayer.h"

#include <functional>

MelodyPlayer::MelodyPlayer(gpio_num_t buzzerPin)
    : m_buzzer(create_buzzer(buzzerPin))
{ }

MelodyPlayer::~MelodyPlayer()
{
    if (m_thread.joinable())
        m_thread.join();
}

void MelodyPlayer::Play(const note_t* begin, const note_t* end)
{
    auto play_impl = std::bind(&MelodyPlayer::play_impl, this, std::placeholders::_1, std::placeholders::_2);

    m_progress = 0;
    m_thread = std::jthread(play_impl, begin, end);
}

void MelodyPlayer::Stop()
{
    if (m_thread.get_stop_source().stop_possible())
        m_thread.get_stop_source().request_stop();
}

void MelodyPlayer::play_impl(const note_t* begin, const note_t* end)
{
    for (auto note = begin;
         note != end && !m_thread.get_stop_token().stop_requested();
         ++note, ++m_progress)
    {
        int freq = get<0>(*note);
        int dura = get<1>(*note);
        if (freq) {
            // Non-blocking - plays in background
            m_buzzer.Beep(freq, dura);
        }
        // Since Beep is non-blocking, we need to wait for the duration of note
        vTaskDelay(pdMS_TO_TICKS(dura));
    }
}

Player MelodyPlayer::create_buzzer(gpio_num_t pin) {
    ledc_timer_config_t timer_config = {};
    ledc_channel_config_t channel_config = {};
    Player::MakeConfig(&timer_config, &channel_config, pin);
    return Player(&timer_config, &channel_config);
}

