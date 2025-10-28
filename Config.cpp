// Config.cpp
#include "Config.h"
#include "Hid.h"  // <-- include HID definitions

// Debug flags enabled at boot
uint8_t DEBUG_LEVEL = OLED_DEBUG | PCF_DEBUG | RADIO_DEBUG | ROLE_DEBUG | HID_DEBUG | FS_DEBUG;

// Encryption key (must match on both ends)
uint8_t ENCRYPTKEY[16] = {
  '1', '6', 'B', 'y', 't', 'e', 'S', 'e', 'c', 'r', 'e', 't', 'K', 'e', 'y', '!'
};

// Human-friendly pin names for PCF8575 input mapping
const char* PIN_NAMES[16] = {
  "P0", "P1", "P2", "P3", "P4", "P5", "P6", "P7",
  "P8", "P9", "P10", "P11", "P12", "P13", "P14", "P15"
};

// Optional inversion mask for button polarity
uint16_t PCF_INVERT_MASK = 0x0000;

// ======================================================
// HID Mappings: what each PCF button sends to the host
// ======================================================
//
// Each entry = one PCF button (BTN0 → BTN15).
// HidBinding format:
//   { { {type, code, modifiers}, ... up to 4 }, firstDelay, nextDelay }
//
// type: HID_KEYBOARD, HID_MOUSE, HID_GAMEPAD
// code: HID_KEY_* for keyboard, MOUSE_BUTTON_* for mouse, index for gamepad
// modifiers: e.g. KEYBOARD_MODIFIER_LEFTCTRL
// firstDelay: ms before first repeat
// nextDelay: ms between repeats
//


// ======================================================
// Example: Gamepad-only hidMap[]
// - BTN0 → Gamepad "A"
// - BTN1 → Gamepad "B"
// - BTN2 → Gamepad "X"
// - BTN3 → Gamepad "Y"
// - BTN4 → Gamepad D-Pad Left
// - BTN5 → Gamepad D-Pad Up
// - BTN6 → Gamepad D-Pad Right
// - BTN7 → Gamepad D-Pad Down
//
// Repeat behavior: 400 ms before first repeat, 250 ms between repeats
// ======================================================






// ==========================================================
// HID maps for multiple TX nodes (IDs 1..MAX_TX)
// ==========================================================

HidBinding hidMap[MAX_TX][BTN_COUNT] = {
  // ────────────────────────────────
  // TX1 — Gamepad standard layout (original)
  // ────────────────────────────────
  {
    // BTN0 → Gamepad A
    { { { HID_GAMEPAD, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN1 → Gamepad B
    { { { HID_GAMEPAD, 1, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN2 → Gamepad X
    { { { HID_GAMEPAD, 2, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN3 → Gamepad Y
    { { { HID_GAMEPAD, 3, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN4 → D-Pad Left
    { { { HID_GAMEPAD, 4, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN5 → D-Pad Up
    { { { HID_GAMEPAD, 5, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN6 → D-Pad Right
    { { { HID_GAMEPAD, 6, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN7 → D-Pad Down
    { { { HID_GAMEPAD, 7, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // Remaining buttons disabled
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },  // BTN8–BTN15
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 } },

  // ────────────────────────────────
  // TX2 — Keyboard: WSAD + 1234
  // ────────────────────────────────
  {
    // BTN0 → '1' key
    { { { HID_KEYBOARD, HID_KEY_1, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN1 → '2' key
    { { { HID_KEYBOARD, HID_KEY_2, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN2 → '3' key
    { { { HID_KEYBOARD, HID_KEY_3, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN3 → '4' key
    { { { HID_KEYBOARD, HID_KEY_4, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN4 → 'A' key (Left)
    { { { HID_KEYBOARD, HID_KEY_A, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN5 → 'W' key (Up)
    { { { HID_KEYBOARD, HID_KEY_W, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN6 → 'D' key (Right)
    { { { HID_KEYBOARD, HID_KEY_D, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN7 → 'S' key (Down)
    { { { HID_KEYBOARD, HID_KEY_S, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // Remaining buttons disabled
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },  // BTN8–BTN15
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 } },

  // ────────────────────────────────
  // TX3 — Keyboard: IKJL + 5678
  // ────────────────────────────────
  {
    // BTN0 → '5' key
    { { { HID_KEYBOARD, HID_KEY_5, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN1 → '6' key
    { { { HID_KEYBOARD, HID_KEY_6, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN2 → '7' key
    { { { HID_KEYBOARD, HID_KEY_7, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN3 → '8' key
    { { { HID_KEYBOARD, HID_KEY_8, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN4 → 'J' key (Left)
    { { { HID_KEYBOARD, HID_KEY_J, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN5 → 'I' key (Up)
    { { { HID_KEYBOARD, HID_KEY_I, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN6 → 'L' key (Right)
    { { { HID_KEYBOARD, HID_KEY_L, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN7 → 'K' key (Down)
    { { { HID_KEYBOARD, HID_KEY_K, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // Remaining buttons disabled
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },  // BTN8–BTN15
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 } },

  // ────────────────────────────────
  // TX4 — Mouse: Movement + Clicks + Key '9'
  // ────────────────────────────────
  {
    // BTN0 → Mouse Left Click
    { { { HID_MOUSE, MOUSE_BUTTON_LEFT, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN1 → Mouse Middle Click
    { { { HID_MOUSE, MOUSE_BUTTON_MIDDLE, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN2 → Mouse Right Click
    { { { HID_MOUSE, MOUSE_BUTTON_RIGHT, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN3 → '9' key
    { { { HID_KEYBOARD, HID_KEY_9, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN4 → Move Left
    { { { HID_MOUSE_AXIS, 0, (uint8_t)-10 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN5 → Move Up
    { { { HID_MOUSE_AXIS, 1, (uint8_t)-10 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN6 → Move Right
    { { { HID_MOUSE_AXIS, 0, (uint8_t)10 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // BTN7 → Move Down
    { { { HID_MOUSE_AXIS, 1, (uint8_t)10 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 400, 250 },

    // Remaining buttons disabled
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },  // BTN8–BTN15
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 },
    { { { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 }, { HID_NONE, 0, 0 } }, 0, 0 } }
};
