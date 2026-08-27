#include <Arduino.h>
#include <Wire.h>

#include "pins.h"
#include "config.h"
#include "led.h"
#include "relay.h"
#include "rtc.h"
#include "eeprom24c32.h"
#include "alarm.h"
#include "button.h"
#include "wifi_ap.h"
#include "web_server.h"

static bool in_config_mode = false;
static unsigned long config_inactivity_timeout = 0;
static bool config_exit_requested = false;

// Any real activity in config mode (a web request, not just the button)
// pushes the inactivity shutoff back out. Called from web_server.cpp too.
void configModeTouch()
{
	config_inactivity_timeout = millis() + CONFIG_MODE_TIMEOUT_MS;
}

// Called from web_server.cpp when the page's "Salir"/"Guardar" buttons are
// tapped. Just sets a flag rather than tearing the server down directly —
// this runs from inside a request handler, itself called from
// webServerUpdate(), so stopping the server mid-callback would mean pulling
// the rug out from under its own call stack. loop() picks this up once
// webServerUpdate() has returned and it's safe to shut down.
void requestConfigModeExit()
{
	config_exit_requested = true;
}

static void enterConfigMode()
{
	in_config_mode = true;
	configModeTouch();
	ledStartFlashTeal();
	wifiApStart();
	webServerStart();
	Serial.println("[config] entered");
}

static void exitConfigMode()
{
	in_config_mode = false;
	ledStopFlash();
	webServerStop();
	wifiApStop();
	Serial.println("[config] exited (timeout or explicit)");
}

static uint8_t lastCheckedMinute = 255;

static void checkAlarms(const DateTimeFields &now)
{
	if (now.minute == lastCheckedMinute)
	{
		return; // only need to check once per minute
	}
	lastCheckedMinute = now.minute;

	for (uint8_t slot = 0; slot < MAX_ALARMS; slot++)
	{
		Alarm a;
		if (!alarmGet(slot, &a))
		{
			continue;
		}
		if (alarmMatches(a, now))
		{
			Serial.printf("[alarm] slot %u fired at %02u:%02u\n", slot, now.hour, now.minute);
			relayPulse();
		}
	}
}

void setup()
{
	Serial.begin(115200);
	delay(200);

	Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

	ledInit();
	ledBootDance();
	relayInit();
	buttonInit();

	if (!rtcInit())
	{
		Serial.println("[rtc] DS3231M not found!");
	}
	else if (rtcLostPower())
	{
		Serial.println("[rtc] lost power — time needs to be set (e.g. from the config page)");
	}
	else
	{
		DateTimeFields now = rtcNow();
		Serial.printf("[rtc] ok, current time %04u-%02u-%02u %02u:%02u:%02u\n",
					  now.year, now.month, now.day, now.hour, now.minute, now.second);
	}

	if (!eepromInit())
	{
		Serial.println("[eeprom] 24C32N not found!");
	}
	else
	{
		Serial.println("[eeprom] ok");
	}
}

void loop()
{
	buttonUpdate();
	ledUpdate();
	relayUpdate();
	ledSetRinging(relayIsOn());
	ledSetButtonHeld(buttonState());

	if (buttonEventPressed())
	{
		if (in_config_mode)
		{
			exitConfigMode();
		}
		else
		{
			enterConfigMode();
		}
	}

	if (in_config_mode && (long)(millis() - config_inactivity_timeout) >= 0)
	{
		exitConfigMode(); // inactivity timeout
	}

	if (in_config_mode)
	{
		webServerUpdate();
	}

	if (config_exit_requested)
	{
		config_exit_requested = false;
		if (in_config_mode)
		{
			exitConfigMode();
		}
	}

	static unsigned long lastRtcPollMs = 0;
	unsigned long now = millis();
	if (now - lastRtcPollMs >= 1000)
	{
		lastRtcPollMs = now;
		checkAlarms(rtcNow());
	}
}
