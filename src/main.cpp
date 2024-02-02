// IMPLEMENT MODE: SHOW BATTERY LEVEL
// IMPLEMENT MODE: SHOW BRIGHTNESS (ALL LIT WHITE)
// IMPLEMENT PAUSE MODE?

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Debounce.h> //       https://github.com/chischte/debounce-library
#include <EEPROM_Counter.h> // https://github.com/chischte/eeprom-counter-library.git

// GLOBALS ---------------------------------------------------------------------

int PIN_GND = 2;
int PIN_RIGHT = 3;
int PIN_DOWN = 4;
int PIN_LEFT = 5;
int PIN_CENTER = 6;
int PIN_UP = 7;

// MODIFY TO CHANGE BEHAVIOUR:
const int runtime_increment = 300; //[s]
bool timer_is_running = false; // if set to true, timer runs after power on
byte brightness_levels[] = {1, 2, 3, 5, 9, 15, 27, 48, 85, 150};
byte no_of_brightness_levels;
byte current_brightness_level;

// FIXED:
unsigned long runtime_secs; // gets read from eeprom during setup
unsigned long start_time; //[ms]

// DEBOUNCE INPUT PINS ---------------------------------------------------------

Debounce switch_right(PIN_RIGHT);
Debounce switch_down(PIN_DOWN);
Debounce switch_left(PIN_LEFT);
Debounce switch_center(PIN_CENTER);
Debounce switch_up(PIN_UP);

enum enum_switch_state {
  state_right, //
  state_down, //
  state_left, //
  state_center, //
  state_up, //
  end_of_enum
};

// EEPROM ----------------------------------------------------------------------

enum counter {
  stored_brightness_level, //
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

  byte max_level = no_of_brightness_levels - 1;

  if (current_brightness_level < max_level) {
    current_brightness_level += 1;
  } else {
    current_brightness_level = max_level;
  }
  byte brightness = brightness_levels[current_brightness_level];
  ring.setBrightness(brightness);
  ring.show();

  // CONSOLE OUT:
  Serial.print("BRIGHTNESS LEVEL: ");
  Serial.println(current_brightness_level);
}

void decrease_brightness() {

  const int min_level = 0;

  if (current_brightness_level > min_level) {
    current_brightness_level -= 1;
  } else {
    current_brightness_level = min_level;
  }
  byte brightness = brightness_levels[current_brightness_level];
  ring.setBrightness(brightness);
  ring.show();

  // CONSOLE OUT:
  Serial.print("BRIGHTNESS LEVEL: ");
  Serial.println(current_brightness_level);
}

void handle_input_chars() {
  int switch_state = end_of_enum;

  if (switch_right.switched_low()) {
    switch_state = state_right;
  }
  if (switch_down.switched_low()) {
    switch_state = state_down;
  }
  if (switch_left.switched_low()) {
    switch_state = state_left;
  }
  if (switch_center.switched_low()) {
    switch_state = state_center;
  }
  if (switch_up.switched_low()) {
    switch_state = state_up;
  }
  switch (switch_state) {
  case state_left:
    timer_is_running = false;
    ring.clear();
    decrease_time();
    break;

  case state_center:
    timer_is_running = true;
    ring.clear();
    start_time = millis();
    break;

  case state_right:
    timer_is_running = false;
    ring.clear();
    increase_time();
    break;

  case state_up:
    increase_brightness();
    break;

  case state_down:
    decrease_brightness();
    break;

  default:
    break;
  }
}

void get_initial_eeprom_values() {

  // GET BRIGHTNESS:

  byte max_level = no_of_brightness_levels - 1;

  current_brightness_level = eeprom_storage.get_value(stored_brightness_level);

  if (current_brightness_level < 0) {
    current_brightness_level = 0;
  }
  if (current_brightness_level > max_level) {
    current_brightness_level = max_level;
  }

  // GET RUNTIME:
  runtime_secs = eeprom_storage.get_value(stored_duration);

  if (runtime_secs % runtime_increment) {
    runtime_secs = runtime_increment * (runtime_secs / runtime_increment);
  }

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

  static unsigned long prev_brightness_level = current_brightness_level;

  if (prev_brightness_level != current_brightness_level) {
    eeprom_storage.set_value(stored_brightness_level, current_brightness_level);
    prev_brightness_level = current_brightness_level;

    // Serial print:
    byte brightness = brightness_levels[current_brightness_level];
    Serial.print("STORED BRIGHTNESS LEVEL: ");
    Serial.print(current_brightness_level);
    Serial.print(" | STORED BRIGHTNESS: ");
    Serial.println(brightness);
  }
}

// SETUP -----------------------------------------------------------------------

void setup() {

  Serial.begin(9600);

  // ADDITIONAL GND PIN:
  pinMode(PIN_GND, OUTPUT);
  digitalWrite(PIN_GND, LOW);

  no_of_brightness_levels = sizeof(brightness_levels) / sizeof(byte);
  Serial.print("NUMBER OF BRIGHTNESS LEVELS: ");
  Serial.println(no_of_brightness_levels);

  // SET INPUT PINS PULLUP:
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_CENTER, INPUT_PULLUP);
  pinMode(PIN_UP, INPUT_PULLUP);

  // EEPROM:
  eeprom_storage.setup(eeprom_min_address, eepromMaxAddress, number_of_values);
  get_initial_eeprom_values();

  // RING:
  ring.begin();
  ring.show();
  ring.setBrightness(brightness_levels[current_brightness_level]);

  Serial.println("EXIT SETUP");

  start_time = millis();
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
