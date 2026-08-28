#pragma once

// ---- I2C addresses ----
#define RTC_I2C_ADDR        0x68   // DS3231M
#define EEPROM_I2C_ADDR     0x57   // 24C32N (A0-A2 tied low, typical RTC breakout default)
#define OLED_I2C_ADDR       0x3C   // SH1106 128x64 (some boards use 0x3D instead)

// ---- EEPROM (24C32N: 4096 bytes, 32-byte page size) ----
#define EEPROM_SIZE_BYTES   4096
#define EEPROM_PAGE_SIZE    32

// ---- Alarm storage ----
#define ALARM_MAGIC         0xA1
#define ALARM_STORAGE_ADDR  0        // start of alarm table in EEPROM
#define MAX_ALARMS          100      // 500 bytes of the 4096-byte EEPROM, plenty of room left

// ---- Relay / bell ----
#define RELAY_ACTIVE_HIGH   true     // flip to false if the relay module is active-low
#define BELL_RING_MS        4000     // default pulse length for a bell "ring" event

// ---- Button / config mode ----
#define BUTTON_POLL_INTERVAL_MS   50 // 20 Hz; sampling this slowly is itself the debounce
#define CONFIG_MODE_TIMEOUT_MS (5UL * 60UL * 1000UL) // auto-shutoff after 5 min idle

// ---- Config-mode Wi-Fi AP ----
// Fixed name is fine: short range, and units are never in config mode at
// the same time as each other unless someone's actively pressing buttons
// on two of them within earshot.
#define CONFIG_AP_SSID          "Minuette"
#define CONFIG_AP_PASSWORD     "minuta12345" // WPA2 needs >= 8 chars
