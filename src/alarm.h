#pragma once

#include <stdint.h>
#include "rtc.h"

// Days-of-week bitmask: bit N set means "fires on that day".
// Bit index matches DateTimeFields::dayOfWeek (0=Sunday .. 6=Saturday).
#define DOW_SUN (1 << 0)
#define DOW_MON (1 << 1)
#define DOW_TUE (1 << 2)
#define DOW_WED (1 << 3)
#define DOW_THU (1 << 4)
#define DOW_FRI (1 << 5)
#define DOW_SAT (1 << 6)
#define DOW_WEEKDAYS (DOW_MON | DOW_TUE | DOW_WED | DOW_THU | DOW_FRI)
#define DOW_ALL 0x7F

struct Alarm {
  bool enabled;
  uint8_t hour;     // 0-23
  uint8_t minute;   // 0-59
  uint8_t daysMask; // DOW_* bits, ignored when `once` is set
  bool once;        // fire at most once, then the caller deletes the slot
};

// Alarms live in fixed slots [0, MAX_ALARMS) on the 24C32N EEPROM.
// Deleting an alarm just frees its slot; slots are otherwise stable so a
// slot index can be used as a stable "alarm id" by the web UI.

bool alarmGet(uint8_t slot, Alarm *out);      // false if slot is empty/out of range
bool alarmSet(uint8_t slot, const Alarm &a);  // writes/overwrites a slot
bool alarmDelete(uint8_t slot);               // frees a slot

// Finds a free slot and writes the alarm into it. Returns the slot index,
// or -1 if every slot is full.
int alarmAdd(const Alarm &a);

// True if this alarm's schedule matches the given time (to the minute) and
// it's enabled and scheduled for that day of the week — unless `once` is
// set, in which case the day of week is ignored and any day matches.
bool alarmMatches(const Alarm &a, const DateTimeFields &now);
