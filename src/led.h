#pragma once

// Status LED: single WS2812 on PIN_WS2812.
// Off by default; flashes teal while config mode / the Wi-Fi AP is active.

void ledInit();

// Call every loop() iteration; drives the non-blocking flash animation.
void ledUpdate();

// Blocking startup animation: cycles red, orange, green, cyan, blue, purple.
// Call once from setup(), after ledInit().
void ledBootDance();

void ledOff();
void ledSetColor(uint8_t r, uint8_t g, uint8_t b);

// Starts/stops a blinking teal indicator (used for config mode).
void ledStartFlashTeal();
void ledStopFlash();

// Solid white while the bell is ringing, overriding whatever else the LED
// was doing (e.g. the config-mode teal flash); resumes it cleanly once
// ringing stops. Call every loop() iteration with the relay's current
// on/off state — it only actually does anything on a state change.
void ledSetRinging(bool ringing);
