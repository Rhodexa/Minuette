#include <Arduino.h>

#include "button.h"
#include "pins.h"
#include "config.h"

static bool lastStableState = false; // true = pressed
static bool lastRawState = false;
static unsigned long lastChangeMs = 0;
static bool pressEvent = false;

static inline bool readPressed() {
  int level = digitalRead(PIN_BUTTON);
  return BUTTON_ACTIVE_LOW ? (level == LOW) : (level == HIGH);
}

void buttonInit() {
  pinMode(PIN_BUTTON, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  lastRawState = readPressed();
  lastStableState = lastRawState;
  lastChangeMs = millis();
}

void buttonUpdate() {
  bool raw = readPressed();
  unsigned long now = millis();

  if (raw != lastRawState) {
    lastRawState = raw;
    lastChangeMs = now;
  }

  if ((now - lastChangeMs) >= BUTTON_DEBOUNCE_MS && lastStableState != lastRawState) {
    lastStableState = lastRawState;
    if (lastStableState) {
      pressEvent = true; // debounced press (falling edge into "pressed")
    }
  }
}

bool buttonWasPressed() {
  if (pressEvent) {
    pressEvent = false;
    return true;
  }
  return false;
}
