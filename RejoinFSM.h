#pragma once
#include "Config.h"

// ────────────────────────────────
// Forward declaration to break circular include dependency
// (Radio.h includes Peers.h → RejoinFSM.h → Radio.h ...)
// ────────────────────────────────
class Radio;

// Optional: forward declare Role enum if not included yet
enum class Role : uint8_t;

// ────────────────────────────────
// RejoinFSM class — manages link state for RX peer entries
// ────────────────────────────────
enum class LinkState : uint8_t {
  DOWN,
  JOINING,
  UP
};

class RejoinFSM {
public:
  LinkState link = LinkState::DOWN;
  unsigned long lastPacketTime = 0;
  unsigned long lastHeartbeat = 0;
  unsigned long lastJoinAttempt = 0;

  void begin();
  void notePacket();
  void taskRun(Radio &radio, Role role);
  void taskHeartbeat(Radio &radio, Role role);
};
