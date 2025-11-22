#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define NUM_PIXELS 1           // Number of pixels in the strip
#define RGB_BUILTIN 48         // Pin where the built-in RGB strip is connected

Adafruit_NeoPixel strip(NUM_PIXELS, RGB_BUILTIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting");
  strip.begin();
  strip.show(); // initialize strip to 'off'
}

void loop() {
    static uint16_t hue = 0;

    // Convert hue → RGB and apply to LED
    uint32_t color = strip.ColorHSV(hue, 255, 255);
    strip.setPixelColor(0, color);
    strip.show();

    // Print the HSV value and converted RGB
    uint8_t r, g, b;
    strip.Color(r, g, b, color);
    Serial.printf("H=%u  ->  RGB=(%u,%u,%u)\n", hue, r, g, b);

    // Step size controls smoothness:
    hue += 64;        // try 10, 20, 64, 256 for different speeds
                      // wraps automatically at 65535

    delay(10);        // smoother motion = smaller delay
}