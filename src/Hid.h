//Hid.h
#pragma once
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>


enum HidType : uint8_t {
  HID_NONE,
  HID_KEYBOARD,
  HID_MOUSE,        // Buttons (Left, Right, Middle)
  HID_MOUSE_AXIS,   // Movement (X/Y deltas)
  HID_GAMEPAD
};


struct HidAction {
  HidType type;
  uint8_t code;       // keycode, mouse button, or gamepad button
  uint8_t modifiers;  // keyboard modifiers
};

struct HidBinding {
  HidAction actions[4];  // up to 4 actions per button
  uint16_t firstDelay;   // ms before first repeat
  uint16_t nextDelay;    // ms between repeats
};

struct HidRuntime {
  bool pressed;
  uint32_t pressStart;
  uint32_t nextRepeat;
  const HidBinding* binding;  // active binding for this node/pin (null when idle)
};

extern HidRuntime hidState[MAX_TX][BTN_COUNT];

void hidBegin();
void hidHandlePress(uint8_t pin);
void hidHandleRelease(uint8_t pin);
void hidTask();
void hidHandlePressWithMap(uint8_t txIndex, uint8_t pin, const HidBinding* map);
void hidHandleReleaseWithMap(uint8_t txIndex, uint8_t pin);


// TinyUSB HID object
extern Adafruit_USBD_HID usb_hid;
