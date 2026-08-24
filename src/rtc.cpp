#include <Arduino.h>
#include <RTClib.h>

#include "rtc.h"

static RTC_DS3231 rtc; // DS3231M is register-compatible with RTClib's DS3231 driver

bool rtcInit() {
  return rtc.begin();
}

bool rtcLostPower() {
  return rtc.lostPower();
}

static DateTimeFields fromLib(const DateTime &dt) {
  DateTimeFields f;
  f.year = dt.year();
  f.month = dt.month();
  f.day = dt.day();
  f.hour = dt.hour();
  f.minute = dt.minute();
  f.second = dt.second();
  f.dayOfWeek = dt.dayOfTheWeek(); // 0=Sunday .. 6=Saturday
  return f;
}

DateTimeFields rtcNow() {
  return fromLib(rtc.now());
}

void rtcSet(const DateTimeFields &dt) {
  rtc.adjust(DateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second));
}

DateTimeFields rtcFromUnixTime(uint32_t unixTime) {
  return fromLib(DateTime(unixTime));
}

uint32_t rtcToUnixTime(const DateTimeFields &dt) {
  return DateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second).unixtime();
}
