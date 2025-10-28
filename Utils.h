#pragma once
#include <Arduino.h>

bool i2cScanDevice(uint8_t addr);
String formatPinDelta(uint16_t prev, uint16_t curr);
