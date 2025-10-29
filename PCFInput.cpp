#include "PCFInput.h"
#include "Config.h"
#include "Utils.h"
#include "OledUI.h"

extern OledUI oledUI;  // from main .ino


void PCFInput::begin() {
  if (!pcf.begin()) {
    if (DEBUG_LEVEL & PCF_DEBUG) Serial.println(F("[PCF] init failed"));
    return;
  }
  pcf.write16(0xFFFF);  // inputs with pullups
  pinsState = prev = pcf.read16();
  if (DEBUG_LEVEL & PCF_DEBUG) Serial.println(F("[PCF] ready"));
}




void PCFInput::taskPoll(Radio& radio) {
  uint16_t val = pcf.read16() ^ PCF_INVERT_MASK;
  if (val != pinsState) {
    Packet pkt = {};
    pkt.type = PT_PIN;
    pkt.pins = val;
    radio.sendPacket(pkt, Role::TX);

    if (DEBUG_LEVEL & PCF_DEBUG) {
      Serial.print(F("[PCF] TX I: "));
      for (int i = 0; i < 16; i++) {
        if (((val >> i) & 1) != ((prev >> i) & 1)) {
          Serial.print(PIN_NAMES[i]);
          Serial.print(((val >> i) & 1) == PRESSED_LEVEL ? "V " : "^ ");
        }
      }
      Serial.println();
    }
    pinsState = val;
    prev = val;
    oledUI.markDirty();
  }
}
