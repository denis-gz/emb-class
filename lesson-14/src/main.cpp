#include <Arduino.h>

#define PIN_RED     10
#define PIN_BLUE    7
#define PIN_BUTTON  14

enum class LED_Mode {
    None   = 0,
    Red    = 1,
    Blue   = 2,
    Police = 3,
};

struct LED_Info {
    int pin;        // GPIO pin
    bool state;     // Logical state (on/off)
};

// LED indices in the g_led_state array
int ID_RED  = 0;
int ID_BLUE = 1;
int ID_COUNT = ID_BLUE + 1;

// Initialize initial state
LED_Info g_led_state[] = {
    { PIN_RED, false },
    { PIN_BLUE, false },
};

bool g_last_button_state = false;

// Holds currently chosen LED mode
LED_Mode g_led_mode = {};
uint32_t g_loop_counter = 0;

// Finite state machine used to switch LED states (really needed only in Police mode) 
uint32_t g_mode_fsm = 0;

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
void Cycle_LED_Mode();

// Used for logging
const char* LED_Mode_To_String(LED_Mode mode);

void setup() {
    Serial.begin(115200);

    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
    pinMode(PIN_BUTTON, INPUT);

    Process_Output_Pins();
    Cycle_LED_Mode();
}

void loop() {
    ++g_loop_counter;

    Process_Input_Pins();
    if (Process_State())
        Process_Output_Pins();
}

void Process_Input_Pins() {
    // Read button state every 1000 loop cycles (still fast enough)
    if (g_loop_counter % (1 * 1000))
        return;
    int button_state = digitalRead(PIN_BUTTON);
    if (g_last_button_state != button_state) {
        g_last_button_state = button_state;
        // Switch LED mode when button is depressed
        if (button_state == LOW) {
            Cycle_LED_Mode();
        }
    }
}

void Cycle_LED_Mode() {
    switch (g_led_mode) {
        case LED_Mode::None:
            // Next mode is Red
            g_led_mode = LED_Mode::Red;
            g_period = 1000 * 1000;
            g_mode_fsm = 0;
            break;
        case LED_Mode::Red:
            // Next mode is Blue
            g_led_mode = LED_Mode::Blue;
            g_period = 1000 * 1000;
            g_mode_fsm = 0;
            break;
        case LED_Mode::Blue:
            // Next mode is Police
            g_led_mode = LED_Mode::Police;
            g_period = 100 * 1000;
            g_mode_fsm = 0;
            break;
        case LED_Mode::Police:
            // Next mode is None (off)
            g_led_mode = LED_Mode::None;
            g_period = 0;
            g_mode_fsm = 0;
            g_led_state[ID_RED].state = false;
            g_led_state[ID_BLUE].state = false;
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
            break;
        case LED_Mode::Red:
            // Switch off the blue LED, switch the red LED to the opposite
            g_led_state[ID_RED].state ^= true;
            g_led_state[ID_BLUE].state = false;
            break;
        case LED_Mode::Blue:
            // Switch off the red LED, switch the blue LED to the opposite
            g_led_state[ID_RED].state = false;
            g_led_state[ID_BLUE].state ^= true;
            break;
        case LED_Mode::Police: {
            // In Police mode, we really need to use the Finite State Machine
            switch (g_mode_fsm++) {
                case 0:
                    g_led_state[ID_RED].state = true;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 1:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 2:
                    g_led_state[ID_RED].state = true;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 3:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 4:
                    g_led_state[ID_RED].state = true;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 5:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 6:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = true;
                    break;
                case 7:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 8:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = true;
                    break;
                case 9:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = false;
                    break;
                case 10:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = true;
                    break;
                case 11:
                    g_led_state[ID_RED].state = false;
                    g_led_state[ID_BLUE].state = false;
                    g_mode_fsm = 0;
                    break;
            }
            Serial.printf("FSM state: %d\n", g_mode_fsm);
            break;
        }
    }
    return true;
}

void Process_Output_Pins() {
    for (size_t i = 0; i < ID_COUNT; ++i) {
        digitalWrite(g_led_state[i].pin, g_led_state[i].state ? HIGH : LOW);
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
