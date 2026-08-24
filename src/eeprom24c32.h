#pragma once

#include <stdint.h>
#include <stddef.h>

// Minimal driver for the 24C32N I2C EEPROM (4096 bytes, 32-byte pages),
// as found on typical DS3231 RTC breakout boards.

bool eepromInit();

// addr is a byte offset within the EEPROM (0..EEPROM_SIZE_BYTES-1).
// Handles page-boundary splitting and write-cycle waits internally.
bool eepromWrite(uint16_t addr, const uint8_t *data, size_t len);
bool eepromRead(uint16_t addr, uint8_t *data, size_t len);

bool eepromWriteByte(uint16_t addr, uint8_t value);
bool eepromReadByte(uint16_t addr, uint8_t *value);
