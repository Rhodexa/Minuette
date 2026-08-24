#pragma once

#include <stdint.h>

// Thin wrapper around the DS3231M so callers don't need to know the
// underlying RTC library. Time is exchanged as plain fields — no on-device
// timezone handling, the RTC just holds local wall-clock time.

struct DateTimeFields {
  uint16_t year;
  uint8_t month;   // 1-12
  uint8_t day;     // 1-31
  uint8_t hour;    // 0-23
  uint8_t minute;  // 0-59
  uint8_t second;  // 0-59
  uint8_t dayOfWeek; // 0=Sunday .. 6=Saturday
};

// Returns false if the RTC isn't found on the bus.
bool rtcInit();

// True if the RTC lost power since last set (battery dead / first boot) and
// its time cannot be trusted yet.
bool rtcLostPower();

DateTimeFields rtcNow();
void rtcSet(const DateTimeFields &dt);

// Convenience for the "is the RTC in sync with the phone" banner: builds a
// DateTimeFields from a Unix timestamp (e.g. received from the browser).
DateTimeFields rtcFromUnixTime(uint32_t unixTime);
uint32_t rtcToUnixTime(const DateTimeFields &dt);
