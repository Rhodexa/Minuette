#include "alarm.h"
#include "eeprom24c32.h"
#include "config.h"

// On-EEPROM layout per slot (5 bytes): [valid][enabled][hour][minute][daysMask]
static const uint8_t RECORD_SIZE = 5;

static uint16_t slotAddr(uint8_t slot) {
  return ALARM_STORAGE_ADDR + (uint16_t)slot * RECORD_SIZE;
}

bool alarmGet(uint8_t slot, Alarm *out) {
  if (slot >= MAX_ALARMS) {
    return false;
  }
  uint8_t raw[RECORD_SIZE];
  if (!eepromRead(slotAddr(slot), raw, RECORD_SIZE)) {
    return false;
  }
  if (raw[0] != ALARM_MAGIC) {
    return false; // empty slot
  }
  out->enabled = raw[1] != 0;
  out->hour = raw[2];
  out->minute = raw[3];
  out->daysMask = raw[4];
  return true;
}

bool alarmSet(uint8_t slot, const Alarm &a) {
  if (slot >= MAX_ALARMS) {
    return false;
  }
  uint8_t raw[RECORD_SIZE] = {
    ALARM_MAGIC,
    (uint8_t)(a.enabled ? 1 : 0),
    a.hour,
    a.minute,
    a.daysMask,
  };
  return eepromWrite(slotAddr(slot), raw, RECORD_SIZE);
}

bool alarmDelete(uint8_t slot) {
  if (slot >= MAX_ALARMS) {
    return false;
  }
  return eepromWriteByte(slotAddr(slot), 0x00);
}

int alarmAdd(const Alarm &a) {
  for (uint8_t slot = 0; slot < MAX_ALARMS; slot++) {
    Alarm existing;
    if (!alarmGet(slot, &existing)) {
      if (!alarmSet(slot, a)) {
        return -1;
      }
      return slot;
    }
  }
  return -1;
}

bool alarmMatches(const Alarm &a, const DateTimeFields &now) {
  if (!a.enabled) {
    return false;
  }
  if ((a.daysMask & (1 << now.dayOfWeek)) == 0) {
    return false;
  }
  return a.hour == now.hour && a.minute == now.minute;
}
