#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <EEPROM_Counter.h> // https://github.com/chischte/eeprom-counter-library.git

// GLOBALS ---------------------------------------------------------------------

// MODIFY TO CHANGE BEHAVIOUR:
const int runtime_increment = 300; //[s]
const int max_brightness = 150;

// FIXED:
bool timer_is_running = false;
int brightness; // gets read from eeprom during setup
unsigned long runtime_secs; // gets read from eeprom during setup
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

  // A fully lightened led ring means the time has run out.
  // Therefore the final led should light up when time has run out,
  // not when the last time intervall has started.
  // Therefore the number of intervalls is the number of leds -1.

  float number_of_intervals = number_of_leds - 1;
  float float_time_per_led = float(runtime_secs * 1000) / number_of_intervals;
  unsigned long time_per_led = float_time_per_led;
  return time_per_led;
}

int calculate_current_led() {
  unsigned long time_per_led = calculate_time_per_led();
  unsigned long time_elapsed = millis() - start_time;
  unsigned int current_led = time_elapsed / time_per_led;
  unsigned int highest_index = ring.numPixels() - 1;
  if (current_led >= highest_index) {
    current_led = highest_index;
  }
  return current_led;
}

void fade_all_leds_to_blue() {
  delay(3000); // delay ok, cause function runs only once
  for (int fade_value = 255; fade_value > 1; fade_value--) {
    for (unsigned int pixel_no = 0; pixel_no < ring.numPixels(); pixel_no++) {
      ring.setPixelColor(pixel_no, fade_value, fade_value, 255);
    }
    ring.show();
    delay(1);
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

void show_current_led() {

  int current_led = calculate_current_led();

  ring.setPixelColor(current_led, 255, 255, 255); // Orange: 255/65/0

  ring.show();
}

void run_timer() {
  bool round_completed = millis() - start_time >= (runtime_secs * 1000);

  static bool leds_are_all_blue = false;

  if (!round_completed) {
    show_current_led();
    leds_are_all_blue = false;
  } else {
    if (!leds_are_all_blue) {
      fade_all_leds_to_blue();
      leds_are_all_blue = true;
    }
  }
}

void increase_brightness() {
  int brightness = ring.getBrightness();
  brightness += 1;
  if (brightness > max_brightness) {
    brightness = max_brightness;
  }
  ring.setBrightness(brightness);
  ring.show();

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
  ring.show();

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

void get_eeprom_values() {

  // GET BRIGHTNESS:
  brightness = eeprom_storage.get_value(stored_brightness);

  if (brightness < 1) {
    brightness = 1;
  }
  if (brightness > max_brightness) {
    brightness = max_brightness;
  }

  // GET RUNTIME:
  runtime_secs = eeprom_storage.get_value(stored_duration);

  if (runtime_secs < runtime_increment) {
    runtime_secs = runtime_increment;
  }

  unsigned long max_runtime = runtime_increment * ring.numPixels();

  if (runtime_secs > max_runtime) {
    runtime_secs = max_runtime;
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

  // EEPROM:
  eeprom_storage.setup(eeprom_min_address, eepromMaxAddress, number_of_values);
  get_eeprom_values();

  // RING:
  ring.begin();
  ring.show();
  ring.setBrightness(brightness);

  // SERIAL:
  Serial.begin(9600);
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
