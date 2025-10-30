#include "Config.h"
#include "Hid.h"
#include <type_traits>
#include <utility>

// ====== Globals ======
Adafruit_USBD_HID usb_hid;

// Track per-node/per-pin HID runtime state so repeats work across multiple transmitters
HidRuntime hidState[MAX_TX][BTN_COUNT];  // runtime state per node/pin

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

namespace {
template <typename T, typename = void>
struct has_isInitialized : std::false_type {};

template <typename T>
struct has_isInitialized<T, std::void_t<decltype(std::declval<T&>().isInitialized())>> : std::true_type {};

template <typename T>
void ensureTinyUSBStarted(T& device, std::true_type) {
  if (!device.isInitialized()) {
    device.begin(0);
  }
}

template <typename T>
void ensureTinyUSBStarted(T& device, std::false_type) {
  device.begin(0);
}
}  // namespace

// Helper to test or mutate aggregated HID reports without duplicating logic
static bool keyInReport(uint8_t keycode) {
  for (uint8_t i = 0; i < 6; ++i) {
    if (kbd_report[i] == keycode) return true;
  }
  return false;
}

static bool addKeyToReport(uint8_t keycode) {
  if (keyInReport(keycode)) return true;
  for (uint8_t i = 0; i < 6; ++i) {
    if (kbd_report[i] == 0) {
      kbd_report[i] = keycode;
      return true;
    }
  }
  return false;
}

static bool isKeyHeldElsewhere(uint8_t skipTx, uint8_t skipPin, uint8_t keycode) {
  for (uint8_t tx = 0; tx < MAX_TX; ++tx) {
    for (uint8_t pin = 0; pin < BTN_COUNT; ++pin) {
      if (tx == skipTx && pin == skipPin) continue;
      const HidRuntime &state = hidState[tx][pin];
      if (!state.pressed || state.binding == nullptr) continue;
      for (const HidAction &act : state.binding->actions) {
        if (act.type == HID_KEYBOARD && act.code == keycode) return true;
      }
    }
  }
  return false;
}

static uint8_t modifiersHeldElsewhere(uint8_t skipTx, uint8_t skipPin) {
  uint8_t mask = 0;
  for (uint8_t tx = 0; tx < MAX_TX; ++tx) {
    for (uint8_t pin = 0; pin < BTN_COUNT; ++pin) {
      if (tx == skipTx && pin == skipPin) continue;
      const HidRuntime &state = hidState[tx][pin];
      if (!state.pressed || state.binding == nullptr) continue;
      for (const HidAction &act : state.binding->actions) {
        if (act.type == HID_KEYBOARD) {
          mask |= act.modifiers;
        }
      }
    }
  }
  return mask;
}

static bool isMouseButtonHeldElsewhere(uint8_t skipTx, uint8_t skipPin, uint8_t button) {
  for (uint8_t tx = 0; tx < MAX_TX; ++tx) {
    for (uint8_t pin = 0; pin < BTN_COUNT; ++pin) {
      if (tx == skipTx && pin == skipPin) continue;
      const HidRuntime &state = hidState[tx][pin];
      if (!state.pressed || state.binding == nullptr) continue;
      for (const HidAction &act : state.binding->actions) {
        if (act.type == HID_MOUSE && act.code == button) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool isGamepadButtonHeldElsewhere(uint8_t skipTx, uint8_t skipPin, uint8_t button) {
  for (uint8_t tx = 0; tx < MAX_TX; ++tx) {
    for (uint8_t pin = 0; pin < BTN_COUNT; ++pin) {
      if (tx == skipTx && pin == skipPin) continue;
      const HidRuntime &state = hidState[tx][pin];
      if (!state.pressed || state.binding == nullptr) continue;
      for (const HidAction &act : state.binding->actions) {
        if (act.type == HID_GAMEPAD && act.code == button) {
          return true;
        }
      }
    }
  }
  return false;
}

// ====== Shared helpers ======
static void doPress(uint8_t txIndex, uint8_t pin, const HidBinding &bind) {
  if (txIndex >= MAX_TX || pin >= BTN_COUNT) return;

  HidRuntime &state = hidState[txIndex][pin];
  state.pressed = true;
  state.binding = &bind;
  state.pressStart = millis();
  state.nextRepeat = state.pressStart + bind.firstDelay;

  for (int a = 0; a < 4; a++) {
    const HidAction &act = bind.actions[a];
    if (act.type == HID_NONE) continue;

    switch (act.type) {
      case HID_KEYBOARD:
        if (!addKeyToReport(act.code) && (DEBUG_LEVEL & HID_DEBUG)) {
          Serial.println(F("[HID] keyboard rollover full; drop key"));
        }
        kbd_modifiers |= act.modifiers;
        kbdDirty = true;
        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID] node="));
          Serial.print(txIndex);
          Serial.print(F(" pin="));
          Serial.print(pin);
          Serial.print(F(" press key="));
          Serial.println(act.code);
        }
        break;

      case HID_MOUSE:
        mouse_buttons |= (1 << act.code);
        mouseDirty = true;
        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID] node="));
          Serial.print(txIndex);
          Serial.print(F(" pin="));
          Serial.print(pin);
          Serial.print(F(" press mouse="));
          Serial.println(act.code);
        }
        break;

      case HID_MOUSE_AXIS:
        if (act.code == 0)
          mouse_dx += (int8_t)act.modifiers ? act.modifiers : act.code;
        else if (act.code == 1)
          mouse_dy += (int8_t)act.modifiers ? act.modifiers : act.code;
        else if (act.code == 2)
          mouse_wheel += (int8_t)act.modifiers ? act.modifiers : act.code;
        mouseDirty = true;
        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID] node="));
          Serial.print(txIndex);
          Serial.print(F(" pin="));
          Serial.print(pin);
          Serial.print(F(" move axis="));
          Serial.print(act.code);
          Serial.print(F(" delta="));
          Serial.println(act.modifiers);
        }
        break;

      case HID_GAMEPAD:
        gp_report.buttons |= (1 << act.code);
        gpDirty = true;
        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID] node="));
          Serial.print(txIndex);
          Serial.print(F(" pin="));
          Serial.print(pin);
          Serial.print(F(" press gamepad="));
          Serial.println(act.code);
        }
        break;

      default:
        break;
    }
  }
}

static void doRelease(uint8_t txIndex, uint8_t pin) {
  if (txIndex >= MAX_TX || pin >= BTN_COUNT) return;

  HidRuntime &state = hidState[txIndex][pin];
  const HidBinding *binding = state.binding;
  if (!binding) return;

  const HidBinding &bind = *binding;
  state.pressed = false;
  state.binding = nullptr;
  state.nextRepeat = 0;

  for (int a = 0; a < 4; a++) {
    const HidAction &act = bind.actions[a];
    if (act.type == HID_NONE) continue;

    switch (act.type) {
      case HID_KEYBOARD: {
        bool removed = false;
        if (!isKeyHeldElsewhere(txIndex, pin, act.code)) {
          for (int i = 0; i < 6; ++i) {
            if (kbd_report[i] == act.code) {
              kbd_report[i] = 0;
              removed = true;
              kbdDirty = true;
              break;
            }
          }
        }
        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID] node="));
          Serial.print(txIndex);
          Serial.print(F(" pin="));
          Serial.print(pin);
          Serial.print(F(" release key="));
          Serial.print(act.code);
          if (!removed) Serial.print(F(" (still held)"));
          Serial.println();
        }
        break;
      }

      case HID_MOUSE: {
        uint8_t before = mouse_buttons;
        if (!isMouseButtonHeldElsewhere(txIndex, pin, act.code)) {
          mouse_buttons &= ~(1 << act.code);
        }
        if (mouse_buttons != before) {
          mouseDirty = true;
        }
        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID] node="));
          Serial.print(txIndex);
          Serial.print(F(" pin="));
          Serial.print(pin);
          Serial.print(F(" release mouse="));
          Serial.print(act.code);
          if (mouse_buttons == before) Serial.print(F(" (still held)"));
          Serial.println();
        }
        break;
      }

      case HID_GAMEPAD: {
        uint32_t before = gp_report.buttons;
        if (!isGamepadButtonHeldElsewhere(txIndex, pin, act.code)) {
          gp_report.buttons &= ~(1UL << act.code);
        }
        if (gp_report.buttons != before) {
          gpDirty = true;
        }
        if (DEBUG_LEVEL & HID_DEBUG) {
          Serial.print(F("[HID] node="));
          Serial.print(txIndex);
          Serial.print(F(" pin="));
          Serial.print(pin);
          Serial.print(F(" release gamepad="));
          Serial.print(act.code);
          if (gp_report.buttons == before) Serial.print(F(" (still held)"));
          Serial.println();
        }
        break;
      }

      default:
        break;
    }
  }

  uint8_t modifierMask = modifiersHeldElsewhere(txIndex, pin);
  if (kbd_modifiers != modifierMask) {
    kbd_modifiers = modifierMask;
    kbdDirty = true;
  }
}

// ====== Init ======
void hidBegin() {
  static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2)),
    TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(3))
  };

  ensureTinyUSBStarted(TinyUSBDevice, has_isInitialized<Adafruit_USBD_Device>{});

  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setPollInterval(2);
  usb_hid.begin();

  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  for (uint8_t tx = 0; tx < MAX_TX; ++tx) {
    for (uint8_t pin = 0; pin < BTN_COUNT; ++pin) {
      hidState[tx][pin] = { false, 0, 0, nullptr };
    }
  }
}

// ====== Wrappers for TX (single-node row 0) ======
void hidHandlePress(uint8_t pin) {
  if (pin >= BTN_COUNT) return;
  doPress(0, pin, hidMap[0][pin]);
}

void hidHandleRelease(uint8_t pin) {
  if (pin >= BTN_COUNT) return;
  doRelease(0, pin);
}

// ====== Wrappers for RX multi-node ======
void hidHandlePressWithMap(uint8_t txIndex, uint8_t pin, const HidBinding *map) {
  if (!map || pin >= BTN_COUNT) return;
  doPress(txIndex, pin, map[pin]);
}

void hidHandleReleaseWithMap(uint8_t txIndex, uint8_t pin) {
  if (pin >= BTN_COUNT) return;
  doRelease(txIndex, pin);
}

// ====== Repeat Task ======
void hidTask() {
  // Iterate across every node/pin so RX repeats stay active for all transmitters
  uint32_t now = millis();
  uint8_t repeatQueue[MAX_TX * BTN_COUNT];  // queue repeated keycodes for synthetic keypress pulses
  size_t repeatCount = 0;

  for (uint8_t tx = 0; tx < MAX_TX; ++tx) {
    for (uint8_t pin = 0; pin < BTN_COUNT; ++pin) {
      HidRuntime &state = hidState[tx][pin];
      if (!state.pressed || state.binding == nullptr) continue;
      const HidBinding &bind = *state.binding;
      if (bind.nextDelay == 0) continue;

      if (now >= state.nextRepeat) {
        int8_t dx_accum = 0;
        int8_t dy_accum = 0;
        int8_t wheel_accum = 0;
        bool localMouseDirty = false;

        for (int a = 0; a < 4; a++) {
          const HidAction &act = bind.actions[a];
          if (act.type == HID_NONE) continue;

          switch (act.type) {
            case HID_KEYBOARD:
              kbdDirty = true;
              if (repeatCount < MAX_TX * BTN_COUNT) {
                repeatQueue[repeatCount++] = act.code;
              }
              break;

            case HID_MOUSE:
              localMouseDirty = true;
              break;

            case HID_MOUSE_AXIS:
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

        if (localMouseDirty) {
          mouse_dx = constrain((int16_t)mouse_dx + dx_accum, -127, 127);
          mouse_dy = constrain((int16_t)mouse_dy + dy_accum, -127, 127);
          mouse_wheel = constrain((int16_t)mouse_wheel + wheel_accum, -127, 127);
          mouseDirty = true;

          if (DEBUG_LEVEL & HID_DEBUG) {
            Serial.print(F("[HID REPEAT] node="));
            Serial.print(tx);
            Serial.print(F(" pin="));
            Serial.print(pin);
            Serial.print(F(" delta("));
            Serial.print(dx_accum);
            Serial.print(F(","));
            Serial.print(dy_accum);
            Serial.print(F(","));
            Serial.print(wheel_accum);
            Serial.println(F(")"));
          }
        }

        state.nextRepeat = now + bind.nextDelay;
      }
    }
  }

  if (repeatCount) {
    kbdDirty = true;  // ensure base state is re-sent after synthetic repeats
  }

  if (usb_hid.ready()) {
    if (repeatCount) {
      uint8_t snapshot[6];
      memcpy(snapshot, kbd_report, sizeof(snapshot));

      for (size_t idx = 0; idx < repeatCount; ++idx) {
        uint8_t keycode = repeatQueue[idx];
        uint8_t releaseReport[6];
        memcpy(releaseReport, snapshot, sizeof(releaseReport));

        bool removed = false;
        for (int slot = 0; slot < 6; ++slot) {
          if (releaseReport[slot] == keycode) {
            releaseReport[slot] = 0;
            removed = true;
            break;
          }
        }
        if (!removed) continue;  // key no longer held

        usb_hid.keyboardReport(1, kbd_modifiers, releaseReport);
        usb_hid.keyboardReport(1, kbd_modifiers, snapshot);
      }
    }

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
        Serial.print(mouse_wheel);
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
  } else if (DEBUG_LEVEL & HID_DEBUG) {
    Serial.println(F("[HID] USB not ready for report"));
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
    (void)remote_wakeup_en;
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
