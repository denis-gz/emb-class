#include "ClockAdjuster.h"

#include <esp_log.h>
#include <cstring>

#include "common.h"

ESP_EVENT_DEFINE_BASE(EVENT_CLOCK_ADJUSTER);

enum event_id_t
{
    ROTATE_EVENT_ID,
    CLICK_EVENT_ID,
};

ClockAdjuster::ClockAdjuster(callback_t get_callback, callback_t set_callback)
    : m_display(3)
    , m_get_callback(get_callback)
    , m_set_callback(set_callback)
    , m_encoder(CONFIG_PIN_ENCODER_S1, CONFIG_PIN_ENCODER_S2, [this](bool decrease) { on_rotate(decrease); })
    , m_encoder_key(CONFIG_PIN_ENCODER_KEY, [this] { on_click(); })
{
    ESP_ERROR_CHECK(esp_event_handler_register(
        EVENT_CLOCK_ADJUSTER, ESP_EVENT_ANY_ID,
        member_cast<esp_event_handler_t>(&ClockAdjuster::on_event), this
    ));

    m_display.Start();
    m_encoder.Start();

    update_display();
}

ClockAdjuster::~ClockAdjuster()
{
    m_encoder.Stop();
    m_display.Stop();

    esp_event_handler_unregister(
        EVENT_CLOCK_ADJUSTER, ESP_EVENT_ANY_ID,
        member_cast<esp_event_handler_t>(&ClockAdjuster::on_event)
    );
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

int ClockAdjuster::get_last_day_of_month()
{
    // Here, we take the Unix-time of first day of next month, then subtract 1 second,
    // and thus receive the last day of previous month, which is of out interest.
    tm t = {};
    t.tm_mday = 1,
    t.tm_mon = m_time_info.tm_mon + 1;
    t.tm_year = m_time_info.tm_year;
    time_t ts = mktime(&t) - 1;
    if (tm* p = gmtime(&ts)) {
        return p->tm_mday;
    }
    else {
        return 0;
    }
}

void ClockAdjuster::on_rotate(bool decrease)
{
    esp_event_isr_post(EVENT_CLOCK_ADJUSTER, ROTATE_EVENT_ID, &decrease, sizeof(decrease), nullptr);
}

void ClockAdjuster::on_click()
{
    esp_event_isr_post(EVENT_CLOCK_ADJUSTER, CLICK_EVENT_ID, nullptr, 0, nullptr);
}

void ClockAdjuster::on_event(esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == CLICK_EVENT_ID)
    {
        switch (m_state) {
            case Wait:
                m_get_callback(&m_time_info);
                m_last_day = get_last_day_of_month();
                m_state = Year;
                break;
            case Year:
                m_state = Month;
                break;
            case Month:
                m_state = Day;
                if (m_time_info.tm_mday > m_last_day)
                    m_time_info.tm_mday = m_last_day;
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
                m_time_info.tm_sec = 0;
                m_time_info.tm_yday = 0;
                m_time_info.tm_isdst = 0;
                m_set_callback(&m_time_info);
                m_state = Wait;
                break;
        }
    }
    else if (event_id == ROTATE_EVENT_ID)
    {
        bool decrease = *(bool*) event_data;
        int increment = decrease ? -1 : 1;

        switch (m_state) {
            case Year:
                m_time_info.tm_year += increment;
                break;
            case Month:
                m_time_info.tm_mon += increment;
                if (m_time_info.tm_mon > 11)
                    m_time_info.tm_mon = 0;
                else if (m_time_info.tm_mon < 0)
                    m_time_info.tm_mon = 11;
                m_last_day = get_last_day_of_month();
                break;
            case Day:
                m_time_info.tm_mday += increment;
                if (m_time_info.tm_mday > m_last_day)
                    m_time_info.tm_mday = 1;
                else if (m_time_info.tm_mday < 1)
                    m_time_info.tm_mday = m_last_day;
                break;
            case WeekDay:
                m_time_info.tm_wday += 7 + increment;
                m_time_info.tm_wday %= 7;
                break;
            case Hour:
                m_time_info.tm_hour += increment;
                if (m_time_info.tm_hour > 23)
                    m_time_info.tm_hour = 0;
                else if (m_time_info.tm_hour < 0)
                    m_time_info.tm_hour = 23;
                break;
            case Minute:
                m_time_info.tm_min += increment;
                if (m_time_info.tm_min > 59)
                    m_time_info.tm_min = 0;
                else if (m_time_info.tm_min < 0)
                    m_time_info.tm_min = 59;
                break;
            default:
                break;
        }
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
            snprintf(buf, sizeof(buf), "M%02u", m_time_info.tm_mon + 1);
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
