#pragma once
#include <RH_RF69.h>
#include "Config.h"
#include "Packet.h"
#include "Peers.h"

// ────────────────────────────────
// Airtime tracking structure
// ────────────────────────────────
struct AirEntry {
  uint32_t ts;   // millis timestamp
  uint32_t dur;  // µs duration
};

// ────────────────────────────────
// TX role state machine
// ────────────────────────────────
enum TxMode {
  TX_MODE_EPHEMERAL,
  TX_MODE_ASSIGN_REQ,
  TX_MODE_ASSIGNED
};

// ────────────────────────────────
// RX node registry entry
// ────────────────────────────────
struct NodeEntry {
  uint16_t fingerprint;
  uint8_t nodeId;
  String nodeName;
  int8_t lastRssi;
  uint32_t lastSeen;
  bool assigned;
};

class Radio {
public:
  RH_RF69 rf69 = RH_RF69(RFM69_CS, RFM69_INT);

  // ───── Runtime state ─────
  uint32_t seq = 0;
  long lastRssi = 0;
  float lastTxTime = 0.0f;

  // Airtime tracking
  static const int BUF_SIZE = 64;
  AirEntry airBuf[BUF_SIZE];
  int head = 0;
  int count = 0;
  uint32_t rollingSum_us = 0;

  // ───── TX assignment state ─────
  TxMode txMode = TX_MODE_EPHEMERAL;
  uint16_t txFingerprint = 0;
  uint32_t lastAdvertise = 0;
  uint8_t requestedId = 0;
  uint32_t assignRequestSentAt = 0;
  bool awaitingAssignResponse = false;

  // ───── RX management ─────
  NodeEntry nodeTable[MAX_TX];
  bool allowAutoNaming = true;

  // ───── Public API ─────
  void begin(Role role);
  void task(Role role);
  void sendPacket(Packet &pkt, Role role);

  // Airtime helpers
  void recordAirtime(uint32_t dur_us);
  void computeAirtime(float &last_ms, float &avg_ms, float &duty_pct);

private:
  // Internal role logic
  void taskTx(Role role);
  void taskRx(Role role);

  // RX-specific helpers
  void handleAssignRequest(const AssignRequest &req);
  void sendAssignNack(uint16_t fingerprint, uint8_t reason);
  void updateEphemeralTable(uint16_t fingerprint, int8_t rssi);
};
