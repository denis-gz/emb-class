#include "ClockAdjuster.h"

#include <esp_log.h>
#include <cstring>

#include "common.h"

ClockAdjuster::ClockAdjuster(callback_t get_callback, callback_t set_callback)
    : m_display(3)
    , m_get_callback(get_callback)
    , m_set_callback(set_callback)
    , m_encoder(CONFIG_PIN_ENCODER_S1, CONFIG_PIN_ENCODER_S2, [this](bool decrease) { on_rotate(decrease); })
    , m_encoder_key(CONFIG_PIN_ENCODER_KEY, [this] { on_click(); })
{
    m_display.Start();
    m_encoder.Start();

    update_display();
}

void ClockAdjuster::get_day_of_week(int wday, char* result, size_t len)
{
    switch (wday) {
        case 0:
            std::strncpy(result, "SUN", len);
            break;
        case 1:
            std::strncpy(result, "MON", len);
            break;
        case 2:
            std::strncpy(result, "TUE", len);
            break;
        case 3:
            std::strncpy(result, "WED", len);
            break;
        case 4:
            std::strncpy(result, "THU", len);
            break;
        case 5:
            std::strncpy(result, "FRI", len);
            break;
        case 6:
            std::strncpy(result, "SAT", len);
            break;
    }
}

void ClockAdjuster::on_rotate(bool decrease)
{
    int increment = decrease ? -1 : 1;

    switch (m_state) {
        case Year:
            m_time_info.tm_year += increment;
            break;
        case Month:
            m_time_info.tm_mon += increment;
            break;
        case Day:
            m_time_info.tm_mday += increment;
            break;
        case WeekDay:
            m_time_info.tm_wday += 7 + increment;
            m_time_info.tm_wday %= 7;
            break;
        case Hour:
            m_time_info.tm_hour += increment;
            break;
        case Minute:
            m_time_info.tm_min += increment;
            break;
        default:
            break;
    }
    update_display();
}

void ClockAdjuster::on_click()
{
    switch (m_state) {
        case Wait:
            m_get_callback(&m_time_info);
            ++m_time_info.tm_mon;
            m_state = Year;
            break;
        case Year:
            m_state = Month;
            break;
        case Month:
            m_state = Day;
            break;
        case Day:
            m_state = WeekDay;
            break;
        case WeekDay:
            m_state = Hour;
            break;
        case Hour:
            m_state = Minute;
            break;
        case Minute:
            --m_time_info.tm_mon;
            m_set_callback(&m_time_info);
            m_state = Wait;
            break;
    }
    update_display();
}

void ClockAdjuster::update_display()
{
    char buf[4];
    switch (m_state) {
        case Wait:
            if (!m_display.IsAnimationRunning())
                m_display.PrintWaitIndicator();
            break;
        case Year:
            snprintf(buf, sizeof(buf), "Y%02u", m_time_info.tm_year % 100);
            m_display.PrintText(buf);
            break;
        case Month:
            snprintf(buf, sizeof(buf), "M%02u", m_time_info.tm_mon);
            m_display.PrintText(buf);
            break;
        case Day:
            snprintf(buf, sizeof(buf), "D%02u", m_time_info.tm_mday);
            m_display.PrintText(buf);
            break;
        case WeekDay:
            get_day_of_week(m_time_info.tm_wday, buf, sizeof(buf));
            m_display.PrintText(buf);
            break;
        case Hour:
            snprintf(buf, sizeof(buf), "H%02u", m_time_info.tm_hour);
            m_display.PrintText(buf);
            break;
        case Minute:
            snprintf(buf, sizeof(buf), " %02u", m_time_info.tm_min);
            m_display.PrintText(buf);
            break;
        default:
            break;
    }
}
