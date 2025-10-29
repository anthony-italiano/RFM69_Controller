#include <Wire.h>
#include "Utils.h"
#include "Config.h"

bool i2cScanDevice(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

String formatPinDelta(uint16_t prev, uint16_t curr) {
  String s = "";
  uint16_t mask = prev ^ curr;  // bits that changed
  for (int i = 0; i < 16; i++) {
    if (mask & (1 << i)) {
      s += PIN_NAMES[i];
      bool pressed = ((curr >> i) & 1) == PRESSED_LEVEL;
      s += pressed ? "V " : "^ ";
    }
  }
  return s;
}

// testI2CDevice must appear before detectRole
bool testI2CDevice(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

Role detectRole() {
  bool oledFound = testI2CDevice(OLED_ADDR);
  bool pcfFound = testI2CDevice(PCF8575_ADDR);

  if (DEBUG_LEVEL & OLED_DEBUG) {
    Serial.print(F("OLED @0x3C: "));
    Serial.println(oledFound ? F("found") : F("missing"));
  }
  if (DEBUG_LEVEL & PCF_DEBUG) {
    Serial.print(F("PCF8575 @0x20: "));
    Serial.println(pcfFound ? F("found") : F("missing"));
  }

  Role role = pcfFound ? Role::TX : Role::RX;

  if (DEBUG_LEVEL & ROLE_DEBUG) {
    Serial.print(F("Role determined: "));
    Serial.println(role == Role::TX ? F("TX") : F("RX"));
  }

  return role;
}