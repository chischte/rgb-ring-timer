#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <EEPROM_Counter.h> //    https://github.com/chischte/eeprom-counter-library.git

// GLOBALS ---------------------------------------------------------------------

bool timer_is_running = false;

unsigned long runtime_secs = 3600;
unsigned int runtime_increment = 300; //[s]
unsigned long start_time; //[ms]

// --- EEPROM ------------------------------------------------------------------

enum counter {
  stored_brightness, //
  stored_duration, //
  endOfEnum //as to be the last one!
};
int number_of_values = endOfEnum;
int eeprom_min_address = 0;
int eepromMaxAddress = 1023;

EEPROM_Counter eeprom_storage;

// RGB RING --------------------------------------------------------------------

#define LED_PIN 8
#define LED_COUNT 24

Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// VARIOUS FUNCTIONS -----------------------------------------------------------

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
  float number_of_leds = ring.numPixels();
  float float_time_per_led = float(runtime_secs * 1000) / number_of_leds;
  unsigned long time_per_led = float_time_per_led;
  return time_per_led;
}

int calculate_current_led() {
  unsigned long time_per_led = calculate_time_per_led();
  unsigned long time_elapsed = millis() - start_time;
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

void increase_time() { //
  runtime_secs += runtime_increment;

  if (runtime_secs >= runtime_increment * LED_COUNT) {
    runtime_secs = runtime_increment * LED_COUNT;
  }

  // CONSOLE OUT:
  Serial.print("DURATION: ");
  float duration = float(runtime_secs) / 60.0;
  Serial.print(duration, 1);
  Serial.println(" [min]");
}

void decrease_time() {
  runtime_secs -= runtime_increment;
  if (runtime_secs <= runtime_increment) {
    runtime_secs = runtime_increment;
  }

  // CONSOLE OUT:
  Serial.print("DURATION: ");
  float duration = float(runtime_secs) / 60.0;
  Serial.print(duration, 1);
  Serial.println(" [min]");
}

void show_timer_duration() {
  int leds_to_show = runtime_secs / runtime_increment;
  for (int i = 0; i < leds_to_show; i++) {
    set_led_blue(i);
  }
}

void fade_in_led() {

  int current_led = calculate_current_led();

  static int previous_led = 0;

  static unsigned long fade_start_time = millis();

  if (current_led != previous_led) {
    fade_start_time = millis();
    previous_led = current_led;
  }

  unsigned long fade_time_elapsed = millis() - fade_start_time;

  float fade_factor = float(fade_time_elapsed) / float(calculate_time_per_led());

  if (fade_factor > 1) {
    fade_factor = 1;
  }

  // if (fade_factor < 0.1) {
  //   fade_factor = 0.1;
  // }

  if (current_led == 0) {
    if (fade_factor < 0.5) {
      fade_factor = 0.5;
    }
  }

  // White:
  const int rgb_full_r = 255;
  const int rgb_full_g = 255;
  const int rgb_full_b = 255;

  // Orange:
  // const int rgb_full_r = 255;
  // const int rgb_full_g = 65;
  // const int rgb_full_b = 0;

  fade_factor = 1; // ACTIVATE TO DISABLE FADE

  // Orange faded:
  int rgb_fade_r = float(rgb_full_r) * fade_factor;
  int rgb_fade_g = float(rgb_full_g) * fade_factor;
  int rgb_fade_b = float(rgb_full_b) * fade_factor;

  int fade_led = calculate_current_led();

  ring.setPixelColor(fade_led, rgb_fade_r, rgb_fade_g, rgb_fade_b);

  // delay(2);

  ring.show();
}

void run_timer() {
  bool round_completed = millis() - start_time >= (runtime_secs * 1000);

  if (!round_completed) {
    fade_in_led();
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

  // CONSOLE OUT:
  Serial.print("BRIGHTNESS: ");
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

  // CONSOLE OUT:
  Serial.print("BRIGHTNESS: ");
  Serial.println(ring.getBrightness());
}

char get_input_char() {
  int incoming_byte = 0;
  char incoming_char = '0';

  if (Serial.available() > 0) {
    incoming_byte = Serial.read();
    incoming_char = char(incoming_byte);
  }
  return incoming_char;
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
    timer_is_running = false;
    ring.clear();
    decrease_time();
    break;

  case do_start:
    timer_is_running = true;
    ring.clear();
    start_time = millis();
    break;

  case do_right:
    timer_is_running = false;
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

void manage_eeprom_updates() {

  static unsigned long prev_duration = runtime_secs;

  if (prev_duration != runtime_secs) {
    eeprom_storage.set_value(stored_duration, runtime_secs);
    prev_duration = runtime_secs;
    Serial.println("STORED DURATION");
  }

  static unsigned long prev_brightness = ring.getBrightness();

  if (prev_brightness != ring.getBrightness()) {
    eeprom_storage.set_value(stored_brightness, ring.getBrightness());
    prev_brightness = ring.getBrightness();
    Serial.println("STORED BRIGHTNESS");
  }
}

// SETUP -----------------------------------------------------------------------

void setup() {
  Serial.begin(9600);

  eeprom_storage.setup(eeprom_min_address, eepromMaxAddress, number_of_values);
  long brightness = eeprom_storage.get_value(stored_brightness);
  runtime_secs = eeprom_storage.get_value(stored_duration);

  ring.begin();
  ring.show();
  ring.setBrightness(brightness);

  Serial.println("EXIT SETUP");
}

// LOOP ------------------------------------------------------------------------

void loop() {

  handle_input_chars();

  manage_eeprom_updates();

  if (timer_is_running) {
    run_timer();
  } else {
    show_timer_duration();
  }
}
