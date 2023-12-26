#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_PIN 8
#define LED_COUNT 24

// ------------------------------
unsigned long runtime_secs = 5;
// ------------------------------

unsigned long time_per_led;
unsigned long start_time;

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void set_led_orange(int led_no) {
  ring.setPixelColor(led_no, 255, 65, 0);
  ring.show();
}

void set_led_green(int led_no) {
  ring.setPixelColor(led_no, 0, 255, 0);
  ring.show();
}

void calculate_time_per_led() {
  float float_time_per_led;
  float number_of_leds = ring.numPixels();
  float_time_per_led = float(runtime_secs * 1000) / number_of_leds;
  time_per_led = float_time_per_led;
}

int calculate_current_led() {
  unsigned long time_elapsed;
  time_elapsed = millis() - start_time;
  unsigned int current_led = time_elapsed / time_per_led;
  if (current_led >= ring.numPixels() - 1) {
    current_led = ring.numPixels() - 1;
  }
  return current_led;
}

void set_all_led_green() {
  for (unsigned int i = 0; i < ring.numPixels(); i++) {
    ring.setPixelColor(i, 0, 255, 0);
    ring.show();
    delay(5);
  }
}

void setup() {
  Serial.begin(9600);
  ring.begin();
  ring.show();
  ring.setBrightness(8);
  calculate_time_per_led();
  start_time = millis();
  // ring.clear();
}

void loop() {

  bool round_completed = millis() - start_time > (runtime_secs * 1000);

  if (!round_completed) {
    set_led_orange(calculate_current_led());
  } else {
    set_all_led_green();
  }
}
