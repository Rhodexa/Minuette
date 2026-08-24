#include <Arduino.h>

#include "relay.h"
#include "pins.h"
#include "config.h"

static bool relayOn_ = false;
static bool pulseActive = false;
static unsigned long pulseEndMs = 0;

static inline void writeRelay(bool on) {
  digitalWrite(PIN_RELAY, on == RELAY_ACTIVE_HIGH ? HIGH : LOW);
  relayOn_ = on;
}

void relayInit() {
  pinMode(PIN_RELAY, OUTPUT);
  writeRelay(false);
}

void relayOn() {
  pulseActive = false;
  writeRelay(true);
}

void relayOff() {
  pulseActive = false;
  writeRelay(false);
}

bool relayIsOn() {
  return relayOn_;
}

void relayPulse(unsigned long durationMs) {
  if (durationMs == 0) {
    durationMs = BELL_RING_MS;
  }
  writeRelay(true);
  pulseActive = true;
  pulseEndMs = millis() + durationMs;
}

void relayUpdate() {
  if (pulseActive && (long)(millis() - pulseEndMs) >= 0) {
    pulseActive = false;
    writeRelay(false);
  }
}
