#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "led.h"
#include "pins.h"
#include "config.h"

static Adafruit_NeoPixel strip(1, PIN_WS2812, NEO_GRB + NEO_KHZ800);

static bool flashing = false;
static bool flashOn = false;
static unsigned long lastToggleMs = 0;
static const unsigned long FLASH_PERIOD_MS = 500;

void ledInit() {
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  ledOff();
}

void ledOff() {
  strip.setPixelColor(0, 0, 0, 0);
  strip.show();
}

void ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void ledStartFlashTeal() {
  flashing = true;
  flashOn = false;
  lastToggleMs = millis();
}

void ledStopFlash() {
  flashing = false;
  ledOff();
}

void ledUpdate() {
  if (!flashing) {
    return;
  }

  unsigned long now = millis();
  if (now - lastToggleMs >= FLASH_PERIOD_MS) {
    lastToggleMs = now;
    flashOn = !flashOn;
    if (flashOn) {
      ledSetColor(0, 128, 128); // teal
    } else {
      ledSetColor(0, 0, 0);
    }
  }
}
