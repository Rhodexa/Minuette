#include <Arduino.h>
#include <Wire.h>

#include "eeprom24c32.h"
#include "config.h"

// 24C32N write cycle time is up to 5ms per datasheet.
static const uint8_t WRITE_CYCLE_MS = 5;

static bool waitForWriteCycle() {
  unsigned long start = millis();
  while (millis() - start < 20) {
    Wire.beginTransmission(EEPROM_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
      return true; // device ACKed, write cycle finished
    }
    delay(1);
  }
  return false;
}

bool eepromInit() {
  Wire.beginTransmission(EEPROM_I2C_ADDR);
  return Wire.endTransmission() == 0;
}

static bool writePage(uint16_t addr, const uint8_t *data, size_t len) {
  Wire.beginTransmission(EEPROM_I2C_ADDR);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xFF));
  Wire.write(data, len);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(WRITE_CYCLE_MS);
  return waitForWriteCycle();
}

bool eepromWrite(uint16_t addr, const uint8_t *data, size_t len) {
  size_t written = 0;
  while (written < len) {
    uint16_t curAddr = addr + written;
    uint16_t spaceInPage = EEPROM_PAGE_SIZE - (curAddr % EEPROM_PAGE_SIZE);
    size_t chunk = len - written;
    if (chunk > spaceInPage) {
      chunk = spaceInPage;
    }
    if (!writePage(curAddr, data + written, chunk)) {
      return false;
    }
    written += chunk;
  }
  return true;
}

bool eepromRead(uint16_t addr, uint8_t *data, size_t len) {
  Wire.beginTransmission(EEPROM_I2C_ADDR);
  Wire.write((uint8_t)(addr >> 8));
  Wire.write((uint8_t)(addr & 0xFF));
  if (Wire.endTransmission(false) != 0) { // repeated start, no stop
    return false;
  }

  size_t received = 0;
  while (received < len) {
    size_t chunk = len - received;
    if (chunk > 32) {
      chunk = 32; // Wire buffer limit
    }
    size_t got = Wire.requestFrom((int)EEPROM_I2C_ADDR, (int)chunk);
    if (got != chunk) {
      return false;
    }
    for (size_t i = 0; i < chunk; i++) {
      data[received + i] = Wire.read();
    }
    received += chunk;
  }
  return true;
}

bool eepromWriteByte(uint16_t addr, uint8_t value) {
  return eepromWrite(addr, &value, 1);
}

bool eepromReadByte(uint16_t addr, uint8_t *value) {
  return eepromRead(addr, value, 1);
}
