#pragma once

// Debounced reader for the general-purpose/BOOT button (PIN_BUTTON).

void buttonInit();

// Call every loop() iteration.
void buttonUpdate();

// True for exactly one buttonUpdate() cycle when a press is detected
// (debounced falling edge). Consume it to trigger config mode.
bool buttonWasPressed();
