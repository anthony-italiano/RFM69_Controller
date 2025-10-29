#pragma once
#include <PCF8575.h>
#include "Config.h"
#include "Packet.h"
#include "Radio.h"

class PCFInput {
public:
  PCF8575 pcf = PCF8575(PCF8575_ADDR);

  // Encapsulated runtime state
  uint16_t pinsState = 0xFFFF;  // current pin snapshot
  uint16_t prev = 0xFFFF;       // previous snapshot

  void begin();
  void taskPoll(Radio& radio);
};
