#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_PIN 8
#define LED_COUNT 24

// ------------------------------
unsigned long runtime_secs = 5;
// ------------------------------

unsigned int runtime_increment = 1; //[s]

unsigned long start_time;

int incomingByte = 0;

bool clock_is_ticking = false;

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void set_led_orange(int led_no) {
  ring.setPixelColor(led_no, 255, 65, 0);
  ring.show();
}

void set_led_green(int led_no) {
  ring.setPixelColor(led_no, 0, 255, 0);
  ring.show();
}

void set_led_blue(int led_no) {
  ring.setPixelColor(led_no, 0, 0, 255);
  ring.show();
}

unsigned long calculate_time_per_led() {
  float float_time_per_led;
  float number_of_leds = ring.numPixels();
  float_time_per_led = float(runtime_secs * 1000) / number_of_leds;
  unsigned long time_per_led = float_time_per_led;
  return time_per_led;
}

int calculate_current_led() {
  unsigned long time_per_led = calculate_time_per_led();
  unsigned long time_elapsed;
  time_elapsed = millis() - start_time;
  unsigned int current_led = time_elapsed / time_per_led;
  if (current_led >= ring.numPixels() - 1) {
    current_led = ring.numPixels() - 1;
  }
  return current_led;
}

void set_all_led_blue() {
  for (unsigned int i = 0; i < ring.numPixels(); i++) {
    set_led_blue(i);
    delay(5);
  }
}

char get_input_char() {
  char incoming_char = '0';

  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    incoming_char = char(incomingByte);
  }
  return incoming_char;
}

void decrease_time() {
  runtime_secs -= runtime_increment;
  if (runtime_secs <= runtime_increment) {
    runtime_secs = runtime_increment;
  }
}

void increase_time() { //
  runtime_secs += runtime_increment;
}

void show_duration() {
  int leds_to_show = runtime_secs / runtime_increment;
  for (int i = 0; i < leds_to_show; i++) {
    set_led_blue(i);
  }
}

void run_clock() {
  bool round_completed = millis() - start_time > (runtime_secs * 1000);

  if (!round_completed) {
    set_led_orange(calculate_current_led());
  } else {
    set_all_led_blue();
  }
}

void increase_brightness() {
  const int max_brightness = 150;
  int brightness = ring.getBrightness();
  brightness += 1;
  if (brightness > max_brightness) {
    brightness = max_brightness;
  }
  ring.setBrightness(brightness);
  Serial.println(ring.getBrightness());
}

void decrease_brightness() {
  const int min_brightness = 1;
  int brightness = ring.getBrightness();
  brightness -= 1;
  if (brightness < min_brightness) {
    brightness = min_brightness;
  }
  ring.setBrightness(brightness);
  Serial.println(ring.getBrightness());
}

void handle_input_chars() {
  char incoming_char = get_input_char();

  const char do_left = 'a';
  const char do_start = 's';
  const char do_right = 'd';
  const char do_up = 'w';
  const char do_down = 'x';

  switch (incoming_char) {
  case do_left:
    clock_is_ticking = false;
    ring.clear();
    decrease_time();
    break;

  case do_start:
    clock_is_ticking = true;
    ring.clear();
    start_time = millis();
    break;

  case do_right:
    clock_is_ticking = false;
    ring.clear();
    increase_time();
    break;

  case do_up:
    increase_brightness();
    break;

  case do_down:
    decrease_brightness();
    break;

  default:
    break;
  }
}

void setup() {
  Serial.begin(9600);
  // Serial.begin(115200);
  ring.begin();
  ring.show();
  ring.setBrightness(10);
  calculate_time_per_led();
  start_time = millis();
  Serial.println("EXIT SETUP");
  // ring.clear();
}

void loop() {
  handle_input_chars();

  if (clock_is_ticking) {
    run_clock();
  } else {
    show_duration();
  }
}
