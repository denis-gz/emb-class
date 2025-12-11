#include <Arduino.h>

// LED pins use pull-down logic (active high)
#define PIN_RED     10
#define PIN_BLUE    7

// Button pin uses pull-up logic (active low)
#define PIN_BUTTON  14

// Minimal time unit (in loop cycles)
#define TIME_UNIT 1000

const uint32_t SLOW_BLINK_PERIOD = 1000 * TIME_UNIT;
const uint32_t FAST_BLINK_PERIOD = 100 * TIME_UNIT;

#define countof(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

enum class LED_Mode {
    None   = 0,
    Red    = 1,
    Blue   = 2,
    Police = 3,
};

struct PIN_State {
    const int pin;  // GPIO pin
    bool state;     // Logical state (on/off)
};

// LED indices in the g_led_state array
const int ID_RED  = 0;
const int ID_BLUE = 1;

// Initialize initial state
PIN_State g_leds[] = {
    { PIN_RED,  0 },
    { PIN_BLUE, 0 },
};

struct LED_Unit {
    const bool red;
    const bool blue;
};

LED_Unit g_police_pattern[] {
    { 1, 0 },
    { 0, 0 },
    { 1, 0 },
    { 0, 0 },
    { 1, 0 },
    { 0, 0 },
    { 0, 1 },
    { 0, 0 },
    { 0, 1 },
    { 0, 0 },
    { 0, 1 },
    { 0, 0 },
};

// Police mode pattern index
uint8_t g_police_index = 0;

bool g_last_button_state = false;

// Holds currently chosen LED mode
LED_Mode g_led_mode = {};
uint32_t g_loop_counter = 0;

// Period for state machine switches
uint32_t g_period = 0;

// Reads button and switches modes
void Process_Input_Pins();

// Turns all the LEDs on or off according to its logical state
void Process_Output_Pins();

// On each period, prepares each LED logical state according to the mode
// Returns true if the state has changed
bool Process_State();

// Switches LED blinking modes in cycle
void Cycle_LED_Modes();

// Used for logging
const char* LED_Mode_To_String(LED_Mode mode);

void setup() {
    Serial.begin(115200);

    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
    pinMode(PIN_BUTTON, INPUT);

    Process_Output_Pins();
    Cycle_LED_Modes();
}

void loop() {
    ++g_loop_counter;

    Process_Input_Pins();
    if (Process_State())
        Process_Output_Pins();
}

void Process_Input_Pins() {
    // Read button state every time unit
    if (g_loop_counter % TIME_UNIT)
        return;
    int button_state = digitalRead(PIN_BUTTON);
    if (g_last_button_state != button_state) {
        g_last_button_state = button_state;
        // Switch LED mode when button is depressed
        if (button_state == LOW) {
            Cycle_LED_Modes();
        }
    }
}

void Cycle_LED_Modes() {
    switch (g_led_mode) {
        case LED_Mode::None:
            // Next mode is Red
            g_led_mode = LED_Mode::Red;
            g_period = SLOW_BLINK_PERIOD;
            break;
        case LED_Mode::Red:
            // Next mode is Blue
            g_led_mode = LED_Mode::Blue;
            g_period = SLOW_BLINK_PERIOD;
            break;
        case LED_Mode::Blue:
            // Next mode is Police
            g_led_mode = LED_Mode::Police;
            g_period = FAST_BLINK_PERIOD;
            g_police_index = 0;
            break;
        case LED_Mode::Police:
            // Next mode is None (off)
            g_led_mode = LED_Mode::None;
            g_period = 0;
            break;
    }
    Serial.printf("LED mode --> %s\n", LED_Mode_To_String(g_led_mode));
}

// Returns true if the state has changed
bool Process_State() {
    // State switching should occur only when a period has elapsed
    if (g_period && g_loop_counter % g_period)
        return false;

    switch (g_led_mode) {
        case LED_Mode::None:
            g_leds[ID_RED].state = 0;
            g_leds[ID_BLUE].state = 0;
            g_period = UINT32_MAX;  // Don't need to handle state anymore
            break;
        case LED_Mode::Red:
            // Switch off the blue LED, switch the red LED to the opposite
            g_leds[ID_RED].state ^= 1;
            g_leds[ID_BLUE].state = 0;
            break;
        case LED_Mode::Blue:
            // Switch off the red LED, switch the blue LED to the opposite
            g_leds[ID_RED].state = 0;
            g_leds[ID_BLUE].state ^= 1;
            break;
        case LED_Mode::Police: {
            auto unit = g_police_pattern[g_police_index++];
            g_leds[ID_RED].state = unit.red;
            g_leds[ID_BLUE].state = unit.blue;
            if (g_police_index >= countof(g_police_pattern))
                g_police_index = 0;
            break;
        }
    }
    return true;
}

void Process_Output_Pins() {
    for (size_t i = 0; i < countof(g_leds); ++i) {
        digitalWrite(g_leds[i].pin, g_leds[i].state ? HIGH : LOW);
    }
}

const char* LED_Mode_To_String(LED_Mode mode) {
    switch (mode) {
        case LED_Mode::None:   return "None";
        case LED_Mode::Red:    return "Red";
        case LED_Mode::Blue:   return "Blue";
        case LED_Mode::Police: return "Police";
    }
    return "";
}
