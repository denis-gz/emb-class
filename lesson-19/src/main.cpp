#include <Arduino.h>

constexpr const int   ADC_PIN   = 6;          // ADC1_5
constexpr const int   ADC_BITS  = 12;
constexpr const float ADC_RANGE = 1 << ADC_BITS;
constexpr const float A_REF     = 1.1f;       // 1.1V
constexpr const float A_OFFSET  = 0.1f;       // Add 0.1V to the readings
constexpr const float K         = 4.0f;       // Filter param

constexpr const int CONTROL_PIN = 10;

constexpr const uint32_t TIME_UNIT = 1000;
constexpr const uint32_t MEASURE_PERIOD = 10  * TIME_UNIT;
constexpr const uint32_t LOG_PERIOD = 500 * TIME_UNIT;
constexpr const uint32_t PWM_RANGE = TIME_UNIT;

uint64_t g_counter          = 0;
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
    ++g_counter;

    float analogValue = 0;

    if (g_counter % MEASURE_PERIOD == 0) {
        analogValue = A_OFFSET + analogReadMilliVolts(ADC_PIN) / 1000.0f;
        g_analog_averaged = (K - 1) * g_analog_averaged / K + analogValue / K;
        g_pwm_duty = g_analog_averaged / A_REF * PWM_RANGE;
        g_pwm_counter = PWM_RANGE;
    }

    if (g_pwm_counter++ == PWM_RANGE)
        g_pwm_counter = 0;

    // Compute duty cycle
    int pwm_step = g_pwm_counter % PWM_RANGE;
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

    if (g_counter % LOG_PERIOD == 0) {
        Serial.printf("mV: %0.3f, pwm_duty: %u\n", g_analog_averaged, g_pwm_duty);
    }    
}

