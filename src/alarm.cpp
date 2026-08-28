#include "alarm.h"
#include "eeprom24c32.h"
#include "config.h"

// On-EEPROM layout per slot (5 bytes): [valid][flags][hour][minute][daysMask]
// flags packs multiple booleans into one byte so adding one doesn't reshuffle
// every slot's address — old records only ever wrote 0x00/0x01 here, so any
// newly-added flag bit reads back as 0 (its "off"/legacy default) for them.
static const uint8_t RECORD_SIZE = 5;
static const uint8_t FLAG_ENABLED = 1 << 0;
static const uint8_t FLAG_ONCE = 1 << 1;

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
  out->enabled = (raw[1] & FLAG_ENABLED) != 0;
  out->hour = raw[2];
  out->minute = raw[3];
  out->daysMask = raw[4];
  out->once = (raw[1] & FLAG_ONCE) != 0;
  return true;
}

bool alarmSet(uint8_t slot, const Alarm &a) {
  if (slot >= MAX_ALARMS) {
    return false;
  }
  uint8_t flags = (a.enabled ? FLAG_ENABLED : 0) | (a.once ? FLAG_ONCE : 0);
  uint8_t raw[RECORD_SIZE] = {
    ALARM_MAGIC,
    flags,
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
  if (!a.once && (a.daysMask & (1 << now.dayOfWeek)) == 0) {
    return false;
  }
  return a.hour == now.hour && a.minute == now.minute;
}
