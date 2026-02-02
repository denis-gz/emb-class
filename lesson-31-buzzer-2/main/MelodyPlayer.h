#pragma once

#include <soc/gpio_num.h>
#include <thread>

#include "buzzer.hpp"

class MelodyPlayer
{
public:
    typedef std::pair<int, int> note_t;

    MelodyPlayer(gpio_num_t buzzer_pin);
    ~MelodyPlayer();

    void Play(const note_t* begin, const note_t* end);
    void Stop();

    uint Progress() const { return m_progress; }

private:
    Player m_buzzer;
    std::chrono::duration<uint> m_whole_duration;
    std::jthread m_thread;
    std::atomic_uint m_progress;

    void play_note(note_t note);
    void play_impl(const note_t* begin, const note_t* end);

    static Player create_buzzer(gpio_num_t pin);
};
