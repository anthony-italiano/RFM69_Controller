//Peers.cpp
#include "Peers.h"
#include "Config.h"

Peer peers[MAX_TX];  // existing global, unchanged

NodeConfig PeerConfig::self;

void PeerConfig::begin(Role role) {
  Storage::begin();

  NodeConfig cfg;
  if (Storage::loadConfig(cfg)) {
    self = cfg;
  } else {
    self.node_addr = DEFAULT_NODE_ADDR;
    self.node_name = DEFAULT_NODE_NAME;

    if (DEBUG_LEVEL & ROLE_DEBUG) {
      Serial.println(F("[PEERCFG] config.json missing, using defaults"));
    }
  }

  if (DEBUG_LEVEL & ROLE_DEBUG) {
    Serial.print(F("[PEERCFG] role="));
    Serial.print(role == Role::TX ? "TX" : "RX");
    Serial.print(F(" addr="));
    Serial.print(self.node_addr);
    Serial.print(F(" name="));
    Serial.println(self.node_name);
  }
}

uint8_t PeerConfig::getNodeAddr() {
  return self.node_addr;
}

String PeerConfig::getNodeName() {
  return self.node_name;
}

void PeerConfig::setNode(uint8_t addr, const String &name) {
  self.node_addr = addr;
  self.node_name = name;

  if (!Storage::saveConfig(self)) {
    if (DEBUG_LEVEL & ROLE_DEBUG) {
      Serial.println(F("[PEERCFG] save failed"));
    }
  } else {
    if (DEBUG_LEVEL & ROLE_DEBUG) {
      Serial.print(F("[PEERCFG] node saved: addr="));
      Serial.print(addr);
      Serial.print(F(" name="));
      Serial.println(name);
    }
  }
}

// ────────────────────────────────
// New methods for TX node reassign
// ────────────────────────────────

// Explicit save helper
void PeerConfig::save() {
  if (!Storage::saveConfig(self)) {
    if (DEBUG_LEVEL & ROLE_DEBUG) {
      Serial.println(F("[PEERCFG] manual save failed"));
    }
  } else {
    if (DEBUG_LEVEL & ROLE_DEBUG) {
      Serial.println(F("[PEERCFG] manual save OK"));
    }
  }
}

// Cycle to next TX node ID (wraps at MAX_TX)
void PeerConfig::nextNode() {
  uint8_t newAddr = self.node_addr + 1;
  if (newAddr > MAX_TX) newAddr = 1;  // wrap around

  // Construct a new default name based on node number
  String newName = String("TX") + String(newAddr);

  if (DEBUG_LEVEL & ROLE_DEBUG) {
    Serial.print(F("[PEERCFG] cycling to next node: "));
    Serial.println(newName);
  }

  setNode(newAddr, newName);
}
