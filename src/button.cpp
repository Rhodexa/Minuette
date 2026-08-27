#include <Arduino.h>

#include "button.h"
#include "pins.h"
#include "config.h"

static const int LONG_PRESS_FRAMES = 20; // ~1s

static bool last_state = false;
static bool current_state = false;
static unsigned long last_poll = 0;

// could be a bit mask... but meh...
static bool event_pressed = false;
static bool event_released = false;
static bool event_long_press = false;

static int press_timer = 0;


static inline bool btn_get()
{
	return !digitalRead(PIN_BUTTON);
}

void buttonInit()
{
	pinMode(PIN_BUTTON, INPUT_PULLUP);
	last_state = btn_get();
	last_poll = millis();
}

void buttonUpdate()
{
	unsigned long now = millis();
	if (now - last_poll < BUTTON_POLL_INTERVAL_MS) { return; }
	last_poll = now;

	current_state = btn_get();

	if(current_state == 1) {
		press_timer++; // Heads up: overshoots LONG_PRESS_FRAMES preventing event retrigger. 
		if (press_timer == LONG_PRESS_FRAMES){
			event_long_press = true;
		}
	}

	if (current_state != last_state)
	{
		if(current_state == 1)
		{
			event_pressed = true;
		}
		else {
			event_released = true;
			press_timer = 0;
		}
	}
}

bool buttonState() {
	return current_state;
}

bool buttonEventPressed()
{
	if (event_pressed)
	{
		event_pressed = false;
		return true;
	}
	return false;
}

bool buttonEventReleased() {
	if (event_released)
	{
		event_released = false;
		return true;
	}
	return false;
}

bool buttonEventLongPress(){
	if (event_long_press)
	{
		event_long_press = false;
		return true;
	}
	return false;
}