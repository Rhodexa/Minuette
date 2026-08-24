#pragma once

// Bell relay on PIN_RELAY. Polarity is set by RELAY_ACTIVE_HIGH in config.h.

void relayInit();

void relayOn();
void relayOff();
bool relayIsOn();

// Starts a non-blocking timed pulse (e.g. "ring the bell for 3s").
// Call relayUpdate() every loop() iteration to have it turn itself off.
void relayPulse(unsigned long durationMs = 0); // 0 -> BELL_RING_MS default
void relayUpdate();
