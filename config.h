// config.h
#pragma once
#include <Arduino.h>
#include "Hid.h"

// ────────────────────────────────
// General build configuration constants
// ────────────────────────────────
#define MAX_TX 4
#define BTN_COUNT 16
#define OLED_ADDR 0x3C
#define RFM69_CS 16
#define RFM69_INT 21
#define RFM69_RST 17
#define TX_NODE_ID 3


extern HidBinding hidMap[MAX_TX][BTN_COUNT];


// ────────────────────────────────
// Radio bitrate configuration
// ────────────────────────────────
// Options: RH_RF69::GFSK_Rb2Fd5, RH_RF69::GFSK_Rb55Fd50, RH_RF69::GFSK_Rb250Fd250
#define RF69_MODEM_CONFIG RH_RF69::GFSK_Rb250Fd250
#define RF69_BITRATE_KBPS 250  // Informational only, matches config above


// ────────────────────────────────
// Hardware constants
// ────────────────────────────────
#define OLED_ADDR 0x3C
#define PCF8575_ADDR 0x20


#define SCOPE_PIN -1  // optional scope pin (-1 disables)

// ────────────────────────────────
// Debug system
// ────────────────────────────────
extern uint8_t DEBUG_LEVEL;

#define OLED_DEBUG 0b00000001   // OLEDUI.cpp, display updates
#define PCF_DEBUG 0b00000010    // PCFInput.cpp, button polling
#define RADIO_DEBUG 0b00000100  // Radio.cpp, packet send/receive
#define FSM_DEBUG 0b00001000    // RejoinFSM.cpp, link state
#define SCHED_DEBUG 0b00010000  // Scheduler.cpp, task timing
#define FS_DEBUG 0b00100000     // Storage.cpp, LittleFS + JSON
#define HID_DEBUG 0b01000000    // HID.cpp, USB device handling
#define ROLE_DEBUG 0b10000000   // Peers.cpp / Role detection

// ────────────────────────────────
// Error Code Table
// ────────────────────────────────
struct ErrorDef {
  int code;
  const char* message;
  bool sticky;
};

enum ErrorCodes {
  ERR_SAVE_FAIL = 101,
  ERR_LOAD_FAIL = 102,
  ERR_FS_MOUNT_FAIL = 103,
  ERR_CONFIG_MISSING = 104,
  ERR_JSON_PARSE = 203,
  ERR_ASSIGN_DENIED = 204,
  ERR_UNKNOWN = 999
};

static const ErrorDef ERROR_DEFS[] = {
  { ERR_SAVE_FAIL, "SAVE FAIL", true },
  { ERR_LOAD_FAIL, "LOAD FAIL", true },
  { ERR_FS_MOUNT_FAIL, "FS MOUNT FAIL", true },
  { ERR_CONFIG_MISSING, "CONFIG MISSING", false },
  { ERR_JSON_PARSE, "JSON PARSE", false },
  { ERR_ASSIGN_DENIED, "ASSIGN DENIED", false },
  { ERR_UNKNOWN, "UNKNOWN", false }
};
#define ERROR_DEF_COUNT (sizeof(ERROR_DEFS) / sizeof(ERROR_DEFS[0]))

inline const char* lookupErrorMsg(int code) {
  for (int i = 0; i < ERROR_DEF_COUNT; i++) {
    if (ERROR_DEFS[i].code == code) return ERROR_DEFS[i].message;
  }
  return "UNKNOWN";
}


// ────────────────────────────────
// Radio configuration
// ────────────────────────────────
static const float RF69_FREQ_MHZ = 915.0f;
static const int8_t RF69_TX_POWER = 2;
static const bool RF69_IS_HCW = true;

extern uint8_t ENCRYPTKEY[16];  // defined in Config.cpp
// ────────────────────────────────
// Roles & addresses
// ────────────────────────────────
#define DEFAULT_NODE_ADDR 1
#define DEFAULT_NODE_NAME "Default Node"

enum class Role : uint8_t { TX = TX_NODE_ID,
                            RX = 2 };

inline uint8_t myAddress(Role role) {
  return role == Role::TX ? TX_NODE_ID : 2;
}
inline uint8_t peerAddress(Role role) {
  return role == Role::TX ? 2 : TX_NODE_ID;
}


// ────────────────────────────────
// Pin naming & inversion
// ────────────────────────────────
extern const char* PIN_NAMES[16];  // defined in Config.cpp
extern uint16_t PCF_INVERT_MASK;   // defined in Config.cpp
#define PRESSED_LEVEL 0

// ────────────────────────────────
// Task intervals (system tuning knobs)
// ────────────────────────────────
#define OLED_INTERVAL 150
#define PCF_POLL_MS 50
#define HEARTBEAT_MS 10000
#define BEACON_BASE_MS 1000
#define JITTER_MAX_MS 25
#define HELLO_BURST_K 2
#define HELLO_DELTA_MS 40
#define ACK_WINDOW_MS 300
#define LINK_DOWN_MS 5000
