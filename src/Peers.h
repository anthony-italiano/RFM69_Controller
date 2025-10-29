#pragma once
#include "RejoinFSM.h"
#include "Storage.h"
#include "Config.h"

// Existing peer connection state
struct Peer {
  RejoinFSM fsm;
  int8_t lastRssi;
};

// Global peer table (for link tracking)
extern Peer peers[MAX_TX];

// Local node identity/config
namespace PeerConfig {
  extern NodeConfig self;  // node_addr + node_name

  void begin(Role role);   // load config or defaults
  uint8_t getNodeAddr();
  String getNodeName();
  void setNode(uint8_t addr, const String &name);

  // New additions for TX node reassignment
  void nextNode();   // Cycle to next node (wraps, saves)
  void save();       // Explicitly save current self to LittleFS
}
