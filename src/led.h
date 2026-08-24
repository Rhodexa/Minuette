#pragma once

// Status LED: single WS2812 on PIN_WS2812.
// Off by default; flashes teal while config mode / the Wi-Fi AP is active.

void ledInit();

// Call every loop() iteration; drives the non-blocking flash animation.
void ledUpdate();

void ledOff();
void ledSetColor(uint8_t r, uint8_t g, uint8_t b);

// Starts/stops a blinking teal indicator (used for config mode).
void ledStartFlashTeal();
void ledStopFlash();
