#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const int LDR_PIN     = 6;          // ADC1_5
const int RGB_PIN     = 48;         // GPIO48;
const int ADC_BITS    = 12;
const float ADC_RANGE = 1 << ADC_BITS;
const float A_REF     = 1.1f;       // 1.1V
const float K         = 4.0f;       // Filter param

const uint32_t TIME_UNIT = 1000;
const uint32_t MEASURE_PERIOD = 10  * TIME_UNIT;
const uint32_t DISPLAY_PERIOD = 500 * TIME_UNIT;   // Must be multiple of MEASURE_PERIOD
const uint32_t FEEDBACK_PERIOD = 100 * TIME_UNIT;

uint64_t g_counter       = 0;
uint64_t g_rgb_show_time = 0;
uint32_t g_blink_period  = 0;

float g_raw_averaged     = 0;
float g_analog_averaged  = 0;

Adafruit_NeoPixel rgb_strip(1, RGB_PIN);

void setup() {
    Serial.begin(115200);
    delay(500);

    analogReadResolution(ADC_BITS);
    analogSetAttenuation(ADC_0db);
    adcAttachPin(LDR_PIN);

    rgb_strip.begin();
    rgb_strip.setPixelColor(0, 0, 0, 0); // Black (off)
    rgb_strip.show();
}

void loop() {
    ++g_counter;

    int rawValue = 0; 
    float analogValue = 0;

    if (g_counter % MEASURE_PERIOD == 0) {
        rawValue = analogReadRaw(LDR_PIN);
        analogValue = analogReadMilliVolts(LDR_PIN) / 1000.0f;

        g_raw_averaged = (K - 1) * g_raw_averaged / K + rawValue / K;
        g_analog_averaged = (K - 1) * g_analog_averaged / K + analogValue / K;

        g_blink_period = 250 * TIME_UNIT / g_analog_averaged;
        // In a case blinking should go faster now, set sooner show time
        if (g_rgb_show_time > g_counter + g_blink_period)
            g_rgb_show_time = g_counter + g_blink_period;
    }

    if (g_counter % DISPLAY_PERIOD == 0) {
        float calculated = g_raw_averaged / ADC_RANGE * A_REF;
        float deviation = calculated - g_analog_averaged;
        Serial.printf("Pin %d raw value: %0.0f, calculated (V): %0.3f, analog (V): %0.3f, deviation: %0.3f\n",
            LDR_PIN, g_raw_averaged, calculated, g_analog_averaged, deviation);
    }

    if (g_counter % FEEDBACK_PERIOD == 0) {
        if (g_analog_averaged > A_REF / 1.5) {
            rgb_strip.setPixelColor(0, 0, 0, 0);
        }
        else {
            rgb_strip.setPixelColor(0, 255, 255, 255);
            rgb_strip.setBrightness(20);
        }
        rgb_strip.show();
    }
/*
    if (g_counter > g_rgb_show_time) {
        if (rgb_strip.getPixelColor(0)) {
            rgb_strip.setPixelColor(0, 0, 0, 0);
            g_rgb_show_time = g_counter + g_blink_period;  // On time depends on period
        }
        else {
            rgb_strip.setPixelColor(0, 0, 0, 255); // Red, Green, Blue
            rgb_strip.setBrightness(100);
            g_rgb_show_time = g_counter + TIME_UNIT;       // Off time is fixed
        }
        rgb_strip.show();
    }
 */
}

