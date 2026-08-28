#include <Arduino.h>
#include <Wire.h>

#include "pins.h"
#include "config.h"
#include "relay.h"
#include "rtc.h"
#include "eeprom24c32.h"
#include "alarm.h"
#include "button.h"
#include "wifi_ap.h"
#include "web_server.h"
#include "oled.h"
#include "oled_gfx.h"

#include "oled_icon_bell.h"
#include "oled_icon_splash.h"
#include "oled_icon_code_link.h"
#include "oled_icon_config.h"
#include "oled_icon_wifi.h"
#include "oled_icon_pass.h"

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
	oledBufClear();
	oledDrawBitmap(1, 34, oledIconConfig);
	oledBufPrintText(7, 16, "Configuracion...");
	oledBufFlush();
	in_config_mode = true;
	configModeTouch();
	wifiApStart();
	webServerStart();
	Serial.println("[config] entered");
}

static void exitConfigMode()
{
	oledBufClear();
	oledBufPrintText(1, 8, "Saliendo...");
	oledBufFlush();
	in_config_mode = false;
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
			if (a.once)
			{
				alarmDelete(slot); // "solo una vez" — consume the slot so it doesn't fire again
			}
		}
	}
}

void setup()
{
	Serial.begin(115200);
	delay(200);

	Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

	relayInit();
	buttonInit();
	oledInit();
	oledClear();

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







// display statemachine
bool display_needs_update = true;
bool display_cleared = false;

// splash and code
int display_show_splash_timer = 5;
int display_show_code_link_timer = 10;

// display bell
int display_bell_show = 0;

// display config
bool display_config_show = false;
int display_config_mode_animation_timer = 0;

// display_clock_show
bool display_clock_show = true;
unsigned long  display_last_update = 0;
int display_clock_show_frame_counter = 0;
DateTimeFields display_clock_show_data;

void loop()
{
	unsigned long now = millis();

	buttonUpdate();
	relayUpdate();

	if (buttonEventLongPress())
	{
		display_clock_show = true;
		display_needs_update = true;
		if (in_config_mode)
		{
			exitConfigMode();
		}
		else
		{
			display_config_mode_animation_timer = 0;
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

	// draw clock on screen
	display_config_show = in_config_mode;

	if(now - display_last_update >= 1000) {
		display_last_update = now;
		display_needs_update = true;
	}

	if (display_needs_update) {
		display_needs_update = false;
		oledBufClear();
		
		if(display_show_splash_timer) {
			oledDrawBitmap(0, 11, oledIconSplash);
			display_show_splash_timer--;
		}
		else if (display_show_code_link_timer) {
			oledDrawBitmap(1, 32, oledIconCodeLink);
			oledBufPrintText(4, 34, "github.com");
			oledBufPrintText(5, 13, "/Rhodexa/Minuette");
			display_show_code_link_timer--;
		}

		else if(display_config_show) {
			display_config_mode_animation_timer ++;
			if(display_config_mode_animation_timer > 35) {
				display_config_mode_animation_timer = 5;
			}
			if(display_config_mode_animation_timer < 5) {
				oledDrawBitmap(0, 67, oledIconConfig);
				oledBufPrintText(5, 0, "Entrando");
				oledBufPrintText(6, 0, "en modo");
				oledBufPrintText(7, 0, "Configuracion");
			}
			else if (display_config_mode_animation_timer < 15) {
				oledDrawBitmap(1, 0, oledIconWifi);
				oledBufClearPage(6);
				oledBufClearPage(7);
				if(display_config_mode_animation_timer < 8)
					oledBufPrintText(7, 1, "...para configurar...");
				else
					oledBufPrintText(7, 1, "Conectate a mi Wi-Fi!");
			}
			else if (display_config_mode_animation_timer < 25)
				oledDrawBitmap(0, 0, oledIconPass);
			else {
				oledBufPrintText(1, 0, "Wi-Fi: Minuette");
				oledBufPrintText(3, 0, "Calve: minuta12345");
				oledBufPrintText(6, 0, "Link de configuracion:");
				oledBufPrintText(7, 0, "http://192.168.4.1");
			}
			
		}

		else if(display_bell_show > 0) {
			oledDrawBitmap(1, 39, oledIconBell);
			display_bell_show--;
		}
		
		else if(display_clock_show) {
			display_clock_show_data = rtcNow();
			oledDrawClock(display_clock_show_data.hour, display_clock_show_data.minute, display_clock_show_frame_counter & 1);
			display_clock_show_frame_counter++;
			if(display_clock_show_frame_counter > 60) {
				display_clock_show_frame_counter = 0;
				display_clock_show = false;
			}
		}

		if(
			display_clock_show ||
			display_config_show ||
			(display_bell_show > 0)
		) {
			oledBufFlush();
			display_cleared = false;
		}
		else {
			if(!display_cleared) {
				oledBufClear();
				oledBufFlush();
				display_cleared = true;
			} 
		}
	}


	static unsigned long lastRtcPollMs = 0;
	if (now - lastRtcPollMs >= 1000)
	{
		lastRtcPollMs = now;
		checkAlarms(rtcNow());
	}
}
