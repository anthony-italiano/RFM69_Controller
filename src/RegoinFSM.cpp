#include "RejoinFSM.h"
#include "Packet.h"
#include "Radio.h"
#include "Config.h"

// Called periodically to evaluate link state
void RejoinFSM::taskRun(Radio& radio, Role role) {
  if ((millis() - lastPacketTime) > LINK_DOWN_MS) {
    if (link != LinkState::DOWN) {
      link = LinkState::DOWN;
      if (DEBUG_LEVEL & FSM_DEBUG) Serial.println(F("[FSM] link DOWN"));
    }
  } else {
    if (link != LinkState::UP) {
      link = LinkState::UP;
      if (DEBUG_LEVEL & FSM_DEBUG) Serial.println(F("[FSM] link UP"));
    }
  }
}

// Called periodically by TX nodes to send heartbeats
void RejoinFSM::taskHeartbeat(Radio& radio, Role role) {
  if (link == LinkState::UP) {
    Packet pkt = {};
    pkt.type = PT_HB;
    pkt.from = 0;       // TODO: fill with this node's address
    pkt.to   = 0xFF;    // broadcast, or specific RX
    radio.sendPacket(pkt, role);

    if (DEBUG_LEVEL & FSM_DEBUG) Serial.println(F("[FSM] heartbeat sent"));
  }
}

// ======================================================
// New helpers for multi-node support
// ======================================================

// Call this whenever a valid packet is received from a peer
void RejoinFSM::notePacket() {
  lastPacketTime = millis();
  link = LinkState::UP;
  if (DEBUG_LEVEL & FSM_DEBUG) Serial.println(F("[FSM] packet noted, link UP"));
}

// Query current state in a simple way
bool RejoinFSM::isUp() const {
  return link == LinkState::UP;
}
