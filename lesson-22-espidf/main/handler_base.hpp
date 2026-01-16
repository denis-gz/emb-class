#pragma once

#include <atomic>
#include <tuple>

#include <driver/gpio.h>
#include <driver/gpio_filter.h>

class HandlerBase
{
public:
    HandlerBase(const char* tag);
    virtual ~HandlerBase();

    int GetCounter() const { return m_counter; }

    // Prints bufferred samples to the log
    bool LogSamples();

    const char* Tag() const { return m_tag; }

protected:
    const char* const m_tag;

    std::atomic_int m_counter;
    std::atomic_int64_t m_sample_time;
    std::atomic_int64_t m_depress_time;
    std::atomic_int64_t m_release_time;
    std::atomic_int m_level;

    esp_err_t Init(gpio_int_type_t int_type);

private:
    // Index in the m_buffer to store the next sample
    std::atomic_int m_index;

    typedef std::tuple<int, int64_t, int64_t, int64_t, int, int> sample_t;
    sample_t m_buffer[100] = {};

    // Glitch filter handle
    gpio_glitch_filter_handle_t gl_handle = {};

    // Common code
    void Handler();

    // Handler-specific implementation
    virtual void HandlerImpl() = 0;
};
