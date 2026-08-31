#include <Arduino.h>
#include <Wire.h>

#include "pins.h"
#include "config.h"
#include "rtc.h"
#include "eeprom24c32.h"
#include "alarm.h"
#include "button.h"
#include "wifi_ap.h"
#include "web_server.h"

// OLED graphics
#include "oled.h"
#include "oled_gfx.h"
#include "oled_icon_bell.h"
#include "oled_icon_splash.h"
#include "oled_icon_code_link.h"
#include "oled_icon_config.h"
#include "oled_icon_wifi.h"
#include "oled_icon_pass.h"

// You can disable the splashscreen. Useful for faster development boots... 
// or... if you simply hate it.
bool display_splashscreen_disable = false;







/* hardware error flags, they gusually get set on boot if hardware is missing from the I2C lines... or hardware on the I2C is in a problematic state */
/* these are purely informative, and can be used to display errors on the screen or serial port at boot. */
bool error_flag_rtc_lost_power = false;
bool error_flag_rtc_not_found = false;
bool error_flag_eeprom_not_found = false;
bool error_flag_display_not_found = false;








static bool in_config_mode = false;
static unsigned long config_inactivity_timeout = 0;
static bool config_exit_requested = false;

// display statemachine
const int DISPLAY_OFF_TIMEOUT = 60;
bool display_paint_requested = true;
unsigned long display_last_update = 0;
int display_off_timer = DISPLAY_OFF_TIMEOUT;

// display config
bool display_config_show = false;
int display_config_mode_animation_timer = 0;

// display clock
DateTimeFields display_clock_show_data;

// bell timer
const int RING_LENGTH_SHORT = 10;  // 1s
const int RING_LENGTH_NORMAL = 30; // 3s
const int RING_LENGTH_LONGER = 45; // 4.5s

unsigned long last_ringer_timer_tick = 0;
int ringer_ring_length = RING_LENGTH_NORMAL;
int ringer_timer = 0;

static uint8_t lastCheckedMinute = 255;










// Bell relay on PIN_RELAY. Polarity is set by RELAY_ACTIVE_HIGH in config.h.
static inline void relay_set(bool on)
{
	digitalWrite(PIN_RELAY, on == RELAY_ACTIVE_HIGH ? HIGH : LOW);
}






static inline void displayOn()
{
	display_off_timer = DISPLAY_OFF_TIMEOUT;
}








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
	// sometimes, entering config mode takes a little longer... this hints the user to be patient
	oledClearBuffer();
	oledDrawBitmap(1, 34, oledIconConfig);
	oledPrintText(7, 16, "Configuracion...");
	oledPushBuffer();

	in_config_mode = true;
	configModeTouch();
	wifiApStart();
	webServerStart();

	Serial.println("[config] entered");
}

static void exitConfigMode()
{
	// sometimes, exiting config mode takes a little longer... this hints the user to be patient
	oledClearBuffer();
	oledPrintText(1, 8, "Saliendo");
	oledPrintText(2, 8, "modo Configuracion...");
	oledPushBuffer();

	in_config_mode = false;
	webServerStop();
	wifiApStop();

	Serial.println("[config] exited (timeout or explicit)");
}









static void checkAlarms(const DateTimeFields &now)
{
	if (now.minute == lastCheckedMinute) return;
	lastCheckedMinute = now.minute;

	// runs once per minute
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
			relay_set(true);
			ringer_timer = ringer_ring_length;
			displayOn();
			display_paint_requested = true;
			if (a.once)
			{
				alarmDelete(slot); // "solo una vez" — consume the slot so it doesn't fire again
			}
		}
	}
}









void displayShowErrors(){
	// Print errors if any
	oledClearBuffer();
	int line = 0;
	if(error_flag_rtc_lost_power) {
		line++;
		oledPrintText(line, 8, "W: Time might be off");
	}
	
	if(error_flag_rtc_not_found) {
		line++;
		oledPrintText(line, 8, "E: RTC Comms Fail");
	}
	
	if(error_flag_eeprom_not_found) {
		line++;
		oledPrintText(line, 8, "E: EEPROM Comms Fail");
	}
	oledPushBuffer();
	
	if(line) delay(10000);
}

void displayShowSplashScreen()
{
	if(display_splashscreen_disable) return;

	oledClearBuffer();
	oledDrawBitmap(0, 11, oledIconSplash);
	oledPushBuffer();

	delay(5000);

	oledClearBuffer();
	oledDrawBitmap(1, 32, oledIconCodeLink);
	oledPrintText(4, 34, "github.com");
	oledPrintText(5, 13, "/Rhodexa/Minuette");
	oledPushBuffer();

	delay(10000);
}









void setup()
{
	Serial.begin(115200);
	delay(200);

	Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

	pinMode(PIN_RELAY, OUTPUT);
	relay_set(false);
	buttonInit();
	oledInit();
	oledRawClear();

	if (!rtcInit())
	{
		error_flag_rtc_not_found = true;
		Serial.println("[rtc] DS3231M not found!");
	}
	else if (rtcLostPower())
	{
		error_flag_rtc_lost_power = true;
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
		error_flag_eeprom_not_found = true;
		Serial.println("[eeprom] 24C32N not found!");
	}
	else
	{
		Serial.println("[eeprom] ok");
	}

	// Errors are ambiguous... hide them
	//displayShowErrors(); // show errors, if any...
	displayShowSplashScreen();
}








void loop()
{
	unsigned long now = millis();

	buttonUpdate();

	if (buttonEventLongPress())
	{
		displayOn();
		display_paint_requested = true;
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






	// Handle alarms
	static unsigned long lastRtcPollMs = 0;
	if (now - lastRtcPollMs >= 1000)
	{
		lastRtcPollMs = now;
		checkAlarms(rtcNow());
	}

	// Bell timer
	if(now - last_ringer_timer_tick >= 100) {
		if (ringer_timer > 0)
		{
			ringer_timer--;
			if (ringer_timer == 0)
			{
				relay_set(false);
			}
		}
	}





	// Display painting routine:

	display_config_show = in_config_mode;

	if (now - display_last_update >= 1000)
	{
		display_last_update = now;
		display_paint_requested = true;
	}

	if (display_paint_requested)
	{
		display_paint_requested = false;
		if(display_off_timer > 0) {
			oledClearBuffer();
		}

		// Priotity Tree

		// show bell icon if currently sounding
		if (ringer_timer > 0)
		{
			displayOn();
			oledDrawBitmap(1, 39, oledIconBell);
		}

		// Configuration slideshow takes over clock
		else if (display_config_show)
		{
			displayOn();
			display_config_mode_animation_timer++;
			if (display_config_mode_animation_timer > 35)
			{
				display_config_mode_animation_timer = 5;
			}
			if (display_config_mode_animation_timer < 5)
			{
				oledDrawBitmap(0, 67, oledIconConfig);
				oledPrintText(5, 0, "Entrando");
				oledPrintText(6, 0, "en modo");
				oledPrintText(7, 0, "Configuracion");
			}
			else if (display_config_mode_animation_timer < 15)
			{
				oledDrawBitmap(1, 0, oledIconWifi);
				oledClearBufferPage(6);
				oledClearBufferPage(7);
				if (display_config_mode_animation_timer < 8)
					oledPrintText(7, 1, "...para configurar...");
				else
					oledPrintText(7, 1, "Conectate a mi Wi-Fi!");
			}
			else if (display_config_mode_animation_timer < 25)
				oledDrawBitmap(0, 0, oledIconPass);
			else
			{
				oledPrintText(1, 0, "Wi-Fi: Minuette");
				oledPrintText(3, 0, "Clave: minuta12345");
				oledPrintText(6, 0, "Link de configuracion:");
				oledPrintText(7, 0, "http://192.168.4.1");
			}
		}


		// Show clock by default
		else if (display_off_timer > 1)
		{
			display_clock_show_data = rtcNow();
			oledDrawClock(display_clock_show_data.hour, display_clock_show_data.minute, now & 0x400);
		} // show_clock
		
		
		// Display activity timer ran out... turn off
		else if (display_off_timer == 1){
			oledClearBuffer();
			oledPushBuffer();
		}
		
		if (display_off_timer > 0)
		{
			display_off_timer--;
			oledPushBuffer();
		}		
	}
}
