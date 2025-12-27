#include <Arduino.h>

constexpr const int   ADC_PIN   = 6;          // ADC1_5
constexpr const int   ADC_BITS  = 12;
constexpr const float ADC_RANGE = 1 << ADC_BITS;
constexpr const float A_REF     = 0.990f;     // 0.990V (actual measured maximum)
constexpr const float K         = 4.0f;       // Filter param

constexpr const int CONTROL_PIN = 10;

constexpr const uint32_t TIME_UNIT = 1000;                  // 1000 us
constexpr const uint32_t MEASURE_PERIOD = 50  * TIME_UNIT;  // 50 ms
constexpr const uint32_t LOG_PERIOD = 500 * TIME_UNIT;      // 500 ms
constexpr const uint32_t PWM_RANGE = 1 * TIME_UNIT;         // 1 ms

uint64_t g_measure_time     = 0;
uint64_t g_log_time         = 0;

float    g_analog_averaged  = 0;

uint32_t g_pwm_counter      = 0;
uint32_t g_pwm_duty         = 0;
bool     g_control          = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    analogReadResolution(ADC_BITS);
    analogSetAttenuation(ADC_0db);
    adcAttachPin(ADC_PIN);

    pinMode(CONTROL_PIN, OUTPUT);
    digitalWrite(CONTROL_PIN, LOW);
}

void loop() {
    ulong time = micros();

    if (time > g_measure_time) {
        float analogValue = analogReadMilliVolts(ADC_PIN) / 1000.0f;
        g_analog_averaged = (K - 1) * g_analog_averaged / K + analogValue / K;
        g_pwm_duty = std::round(g_analog_averaged / A_REF * PWM_RANGE);
        g_measure_time += MEASURE_PERIOD;
    }

    // Compute duty cycle
    int pwm_step = g_pwm_counter++ % PWM_RANGE;
    if (pwm_step < g_pwm_duty) {
        // High level part
        if (!g_control) {
            g_control = true;
            digitalWrite(CONTROL_PIN, HIGH);
        }
    }
    else {
        // Low level part
        if (g_control) {
            g_control = false;
            digitalWrite(CONTROL_PIN, LOW);
        }
    }

    if (time > g_log_time) {
        Serial.printf("mV: %0.3f, PWM: %0.1f%%\n", g_analog_averaged, g_analog_averaged / A_REF * 100.0);
        g_log_time += LOG_PERIOD;
    }    
}

