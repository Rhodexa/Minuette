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
static bool ringing = false;

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

void ledBootDance() {
  static const uint8_t colors[][3] = {
    {255, 0, 0},    // red
    {255, 60, 0},   // orange
    {0, 255, 0},    // green
    {0, 255, 255},  // cyan
    {0, 0, 255},    // blue
    {160, 0, 255},  // purple
  };
  for (uint8_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
    ledSetColor(colors[i][0], colors[i][1], colors[i][2]);
    delay(150);
  }
  ledOff();
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

void ledSetRinging(bool isRinging) {
  if (isRinging == ringing) {
    return; // no change
  }
  ringing = isRinging;

  if (ringing) {
    ledSetColor(255, 255, 255);
    return;
  }

  // Ringing just ended — resume whatever ledUpdate() was doing before,
  // cleanly, instead of leaving it stuck showing stale white or a
  // mid-cycle flash color.
  if (flashing) {
    flashOn = false;
    lastToggleMs = millis();
    ledSetColor(0, 0, 0);
  } else {
    ledOff();
  }
}

void ledUpdate() {
  if (ringing) {
    return; // stay solid white; skip the normal flash toggling entirely
  }

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
