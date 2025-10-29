//Hid.cpp
#include "Hid.h"
#include "Config.h"

// ====== Globals ======
Adafruit_USBD_HID usb_hid;

HidRuntime hidState[16];  // runtime state

// ====== Keyboard state ======
static uint8_t kbd_report[6] = { 0 };
static uint8_t kbd_modifiers = 0;

// ====== Mouse state ======
static uint8_t mouse_buttons = 0;
static int8_t mouse_dx = 0;
static int8_t mouse_dy = 0;
static int8_t mouse_wheel = 0;

// ====== Gamepad state ======
static hid_gamepad_report_t gp_report = {};

// ====== Dirty flags ======
static bool kbdDirty = false;
static bool mouseDirty = false;
static bool gpDirty = false;

// ====== Shared helpers ======
static void doPress(uint8_t pin, HidBinding &bind) {
  hidState[pin].pressed = true;
  hidState[pin].pressStart = millis();
  hidState[pin].nextRepeat = millis() + bind.firstDelay;

  for (int a = 0; a < 4; a++) {
    if (bind.actions[a].type == HID_NONE) continue;
    HidAction act = bind.actions[a];

    if (act.type == HID_KEYBOARD) {
      for (int i = 0; i < 6; i++) {
        if (kbd_report[i] == 0) {
          kbd_report[i] = act.code;
          break;
        }
      }
      kbd_modifiers |= act.modifiers;
      kbdDirty = true;

      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("HID TX BTN"));
        Serial.print(pin);
        Serial.print(F(" → KBD press code "));
        Serial.println(act.code);
      }

    } else if (act.type == HID_MOUSE) {
      mouse_buttons |= (1 << act.code);
      mouseDirty = true;

      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("HID TX BTN"));
        Serial.print(pin);
        Serial.print(F(" → MOUSE press btn "));
        Serial.println(act.code);
      }
    } else if (act.type == HID_MOUSE_AXIS) {
      // act.code = 0 for X, 1 for Y; act.modifiers unused
      if (act.code == 0)
        mouse_dx += (int8_t)act.modifiers ? act.modifiers : act.code;  // backward compat safe
      else if (act.code == 1)
        mouse_dy += (int8_t)act.modifiers ? act.modifiers : act.code;
      else if (act.code == 2)
        mouse_wheel += (int8_t)act.modifiers ? act.modifiers : act.code;
      else
        ;  // reserved for future axes

      mouseDirty = true;

      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("HID TX BTN"));
        Serial.print(pin);
        Serial.print(F(" → MOUSE move axis "));
        Serial.print(act.code == 0 ? "X" : act.code == 1 ? "Y"
                                                         : "Wheel");
        Serial.print(F(" delta="));
        Serial.println(act.modifiers);
      }

    } else if (act.type == HID_GAMEPAD) {
      gp_report.buttons |= (1 << act.code);
      gpDirty = true;

      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("HID TX BTN"));
        Serial.print(pin);
        Serial.print(F(" → GP press btn "));
        Serial.println(act.code);
      }
    }
  }
}

static void doRelease(uint8_t pin, HidBinding &bind) {
  hidState[pin].pressed = false;

  for (int a = 0; a < 4; a++) {
    if (bind.actions[a].type == HID_NONE) continue;
    HidAction act = bind.actions[a];

    if (act.type == HID_KEYBOARD) {
      for (int i = 0; i < 6; i++) {
        if (kbd_report[i] == act.code) { kbd_report[i] = 0; }
      }
      kbd_modifiers &= ~act.modifiers;
      kbdDirty = true;

      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("HID REL BTN"));
        Serial.print(pin);
        Serial.print(F(" → KBD release code "));
        Serial.println(act.code);
      }

    } else if (act.type == HID_MOUSE) {
      mouse_buttons &= ~(1 << act.code);
      mouseDirty = true;

      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("HID REL BTN"));
        Serial.print(pin);
        Serial.print(F(" → MOUSE release btn "));
        Serial.println(act.code);
      }

    } else if (act.type == HID_GAMEPAD) {
      gp_report.buttons &= ~(1 << act.code);
      gpDirty = true;

      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("HID REL BTN"));
        Serial.print(pin);
        Serial.print(F(" → GP release btn "));
        Serial.println(act.code);
      }
    }
  }
}

// ====== Init ======
void hidBegin() {
  static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2)),
    TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))
  };

  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setPollInterval(2);
  usb_hid.begin();

  for (int i = 0; i < 16; i++) {
    hidState[i] = { false, 0, 0 };
  }
}

// ====== Wrappers for TX (single-node row 0) ======
void hidHandlePress(uint8_t pin) {
  HidBinding &bind = hidMap[0][pin];
  doPress(pin, bind);
}

void hidHandleRelease(uint8_t pin) {
  HidBinding &bind = hidMap[0][pin];
  doRelease(pin, bind);
}

// ====== Wrappers for RX multi-node ======
void hidHandlePressWithMap(uint8_t pin, HidBinding *map) {
  HidBinding &bind = map[pin];
  doPress(pin, bind);
}

void hidHandleReleaseWithMap(uint8_t pin, HidBinding *map) {
  HidBinding &bind = map[pin];
  doRelease(pin, bind);
}

// ====== Repeat Task ======
void hidTask() {
  uint32_t now = millis();

  for (int pin = 0; pin < 16; pin++) {
    if (!hidState[pin].pressed) continue;
    HidBinding &bind = hidMap[0][pin];  // TX-only repeat logic
    if (bind.nextDelay == 0) continue;



    if (now >= hidState[pin].nextRepeat) {
      int8_t dx_accum = 0;
      int8_t dy_accum = 0;
      int8_t wheel_accum = 0;
      bool localMouseDirty = false;

      for (int a = 0; a < 4; a++) {
        if (bind.actions[a].type == HID_NONE) continue;
        // HidAction act = bind.actions[a];
        const HidAction &act = bind.actions[a];

        switch (act.type) {
          case HID_KEYBOARD:
            kbdDirty = true;

            if (DEBUG_LEVEL & HID_DEBUG) {
              Serial.print(F("HID REPEAT BTN"));
              Serial.print(pin);
              Serial.print(F(" → KBD code "));
              Serial.println(act.code);
            }
            break;

          case HID_MOUSE:
            localMouseDirty = true;
            break;

          case HID_MOUSE_AXIS:
            // Batch all axis deltas per repeat frame
            if (act.code == 0) dx_accum += (int8_t)act.modifiers;
            else if (act.code == 1) dy_accum += (int8_t)act.modifiers;
            else if (act.code == 2) wheel_accum += (int8_t)act.modifiers;
            localMouseDirty = true;
            break;

          case HID_GAMEPAD:
            gpDirty = true;
            break;

          default:
            break;
        }
      }
      // Apply batched motion changes atomically
      if (localMouseDirty) {
        mouse_dx += dx_accum;
        mouse_dy += dy_accum;
        mouse_wheel += wheel_accum;
        mouse_dx = constrain(mouse_dx, -127, 127);
        mouse_dy = constrain(mouse_dy, -127, 127);
        mouse_wheel = constrain(mouse_wheel, -127, 127);

        mouseDirty = true;


        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID REPEAT] X="));
          Serial.print(dx_accum);
          Serial.print(F(" Y="));
          Serial.print(dy_accum);
          Serial.print(F(" W="));
          Serial.println(wheel_accum);
        }
      }
      hidState[pin].nextRepeat = now + bind.nextDelay;
    }
  }

  // ====== Flush reports ======
  if (usb_hid.ready()) {

    if (kbdDirty) {
      usb_hid.keyboardReport(1, kbd_modifiers, kbd_report);
      kbdDirty = false;
    }

    if (mouseDirty) {
      if (DEBUG_LEVEL & HID_DEBUG) {
        Serial.print(F("[HID] Mouse moved (dx="));
        Serial.print(mouse_dx);
        Serial.print(F(", dy="));
        Serial.print(mouse_dy);
        Serial.print(F(", wheel="));
        Serial.println(mouse_wheel);
        Serial.println(F(")"));
      }
      usb_hid.mouseReport(2, mouse_buttons, mouse_dx, mouse_dy, mouse_wheel, 0);
      mouse_dx = mouse_dy = mouse_wheel = 0;
      mouseDirty = false;
    }

    if (gpDirty) {
      usb_hid.sendReport(3, &gp_report, sizeof(gp_report));
      gpDirty = false;
    }
  } else {
    // Optional: lightweight debug or retry count
    if (DEBUG_LEVEL & HID_DEBUG) {
      Serial.print(F("[HID] USB not ready for report: "));
      Serial.println(usb_hid.ready());
    }
  }
}


// ====== TinyUSB callbacks ======
extern "C" {
  void tud_mount_cb(void) {
    if (DEBUG_LEVEL & HID_DEBUG) {
      Serial.println(F("[USB] HID mounted and enumerated"));
    }
  }

  void tud_umount_cb(void) {
    memset(kbd_report, 0, sizeof(kbd_report));
    kbd_modifiers = 0;
    mouse_buttons = 0;
    gp_report.buttons = 0;

    if (DEBUG_LEVEL & HID_DEBUG) {
      Serial.println(F("[USB] HID reset state on unmount"));
    }
  }

  void tud_suspend_cb(bool remote_wakeup_en) {
    if (DEBUG_LEVEL & HID_DEBUG) {
      Serial.println(F("[USB] HID suspended"));
    }
  }

  void tud_resume_cb(void) {
    if (DEBUG_LEVEL & HID_DEBUG) {
      Serial.println(F("[USB] HID resumed"));
    }
  }
}
