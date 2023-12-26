#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_PIN 8
#define LED_COUNT 24

// Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ800);
// Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ400);
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  ring.begin();
  ring.show();
  ring.setBrightness(5);

  Serial.println(ring.numPixels());
}

void loop() {

  ring.setPixelColor(0, 250, 0, 0);
  ring.show();

  ring.setPixelColor(1, 0, 250, 0);
  ring.show();

  ring.setPixelColor(2, 0, 0, 250);
  ring.show();

  // for (int i = 0; i < ring.numPixels(); i++) {
  //   ring.setPixelColor(i, 255, 0, 0);
  //   ring.show();
  //   delay(100);
  // }
  // for (int i = ring.numPixels() - 1; i >= 0; i--) {
  //   ring.setPixelColor(i, 0, 0, 0, 0);
  //   ring.show();
  //   // delay(200);
  // }
}
