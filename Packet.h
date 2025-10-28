#pragma once
#include <Arduino.h>

// ────────────────────────────────
// Packet Types
// ────────────────────────────────
enum PacketType : uint8_t {
  PT_PIN = 1,
  PT_HB = 2,
  PT_HELLO = 3,
  PT_HACK = 4,
  PT_ROLE = 5,             // optional new packet type for node identity
  PT_ASSIGN = 6,           // assignment confirmation
  PT_NACK = 7,             // assignment denied
  PT_ADVERTISE = 10,       // TX0 → RX: ephemeral advertisement
  PT_ASSIGN_REQUEST = 11,  // TX0 → RX: request permanent node number
  PT_ASSIGN_ACK = 12,      // RX → TX: assignment successful
  PT_ASSIGN_NACK = 13      // RX → TX: assignment failed
};

// ────────────────────────────────
// Generic 14-byte base packet
// ────────────────────────────────
struct __attribute__((packed)) Packet {
  uint8_t from;          // sender addr (0 for TX0)
  uint8_t to;            // receiver addr (0xFF for broadcast)
  uint8_t type;          // PacketType
  uint8_t rsv;           // reserved
  uint16_t pins;         // PCF8575 state
  uint32_t seq;          // sequence number
  uint16_t air20;        // last TX airtime (0.1ms units)
  uint16_t airtot;       // rolling 20s airtime total (ms)
  uint16_t fingerprint;  // ephemeral 16-bit unique ID (used by TX0)
};

// ────────────────────────────────
// Assignment Request (TX → RX)
// ────────────────────────────────
struct __attribute__((packed)) AssignRequest {
  uint16_t fingerprint;  // matches TX ephemeral ID
  uint8_t requested_id;  // requested TX node number
  char node_name[16];    // optional name (may be empty)
};

// ────────────────────────────────
// Assignment Acknowledgment (RX → TX)
// ────────────────────────────────
struct __attribute__((packed)) AssignAck {
  uint16_t fingerprint;  // echoes TX ephemeral ID
  uint8_t assigned_id;   // assigned TX number
  char node_name[16];    // confirmed name
};

// ────────────────────────────────
// Assignment NACK (RX → TX)
// ────────────────────────────────
enum AssignError : uint8_t {
  ASSIGN_ERR_INUSE = 0x01,
  ASSIGN_ERR_MALFORM = 0x02,
  ASSIGN_ERR_NONAME = 0x03,
  ASSIGN_ERR_SAVE = 0x04,
  ASSIGN_ERR_GENERAL = 0x05
};

struct __attribute__((packed)) AssignNack {
  uint16_t fingerprint;
  uint8_t reason;  // AssignError code
};

// ────────────────────────────────
// Optional: helper for NACK reason lookup
// ────────────────────────────────
inline const __FlashStringHelper* getAssignErrorMsg(uint8_t code) {
  switch (code) {
    case ASSIGN_ERR_INUSE: return F("Node ID in use");
    case ASSIGN_ERR_MALFORM: return F("Malformed request");
    case ASSIGN_ERR_NONAME: return F("Name required");
    case ASSIGN_ERR_SAVE: return F("Save failure");
    default: return F("General error");
  }
}
