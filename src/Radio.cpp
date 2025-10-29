#include "Radio.h"
#include "Utils.h"
#include "Config.h"
#include "Hid.h"
#include "Peers.h"
#include "Storage.h"
#include "OledUI.h"

// ────────────────────────────────
// Initialize radio
// ────────────────────────────────
void Radio::begin(Role role) {
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init()) {
    if (DEBUG_LEVEL & RADIO_DEBUG) Serial.println(F("[RADIO] init failed"));
    while (1);
  }

  rf69.setFrequency(RF69_FREQ_MHZ);
  rf69.setTxPower(RF69_TX_POWER, RF69_IS_HCW);
  rf69.setEncryptionKey(ENCRYPTKEY);
  rf69.setThisAddress(myAddress(role));
  rf69.setHeaderFrom(myAddress(role));
  rf69.setHeaderTo(peerAddress(role));

  if (!rf69.setModemConfig(RF69_MODEM_CONFIG)) {
    if (DEBUG_LEVEL & RADIO_DEBUG)
      Serial.println(F("[RADIO] setModemConfig failed, using default"));
  }

  if (DEBUG_LEVEL & RADIO_DEBUG) {
    Serial.print(F("[RADIO] init OK @ "));
    Serial.print(RF69_FREQ_MHZ);
    Serial.print(F(" MHz, bitrate="));
    Serial.print(RF69_BITRATE_KBPS);
    Serial.println(F(" kbps"));
  }

  // ───── TX startup mode ─────
  if (role == Role::TX) {
    if (PeerConfig::getNodeAddr() == 0) {
      txMode = TX_MODE_EPHEMERAL;
      randomSeed(analogRead(0));
      txFingerprint = random(1, 65535);
      if (DEBUG_LEVEL & RADIO_DEBUG)
        Serial.printf("[TX0] Ephemeral fingerprint 0x%04X\n", txFingerprint);
    } else {
      txMode = TX_MODE_ASSIGNED;
    }
  }
}

// ────────────────────────────────
// Unified periodic task
// ────────────────────────────────
void Radio::task(Role role) {
  if (role == Role::TX)
    taskTx(role);
  else
    taskRx(role);
}

// ────────────────────────────────
// TX Task
// ────────────────────────────────
void Radio::taskTx(Role role) {
  switch (txMode) {
    case TX_MODE_EPHEMERAL: {
      if (millis() - lastAdvertise > 2000) {
        Packet pkt = {};
        pkt.type = PT_ADVERTISE;
        pkt.from = 0;
        pkt.to = 0xFF;
        pkt.fingerprint = txFingerprint;
        sendPacket(pkt, role);
        lastAdvertise = millis();

        if (DEBUG_LEVEL & RADIO_DEBUG)
          Serial.printf("[TX0] ADVERTISE (fp=0x%04X)\n", txFingerprint);
      }
      break;
    }

    case TX_MODE_ASSIGN_REQ: {
      if (!awaitingAssignResponse) {
        AssignRequest req = {};
        req.fingerprint = txFingerprint;
        req.requested_id = requestedId;
        strncpy(req.node_name, PeerConfig::getNodeName().c_str(), sizeof(req.node_name) - 1);
        rf69.send((uint8_t*)&req, sizeof(req));
        assignRequestSentAt = millis();
        awaitingAssignResponse = true;

        if (DEBUG_LEVEL & RADIO_DEBUG)
          Serial.printf("[TX0] Sent ASSIGN_REQ #%d\n", requestedId);
      }

      if (awaitingAssignResponse && millis() - assignRequestSentAt > 1000) {
        awaitingAssignResponse = false;
        txMode = TX_MODE_EPHEMERAL;

        Serial.printf("[TX0] Assignment #%d Denied (timeout)\n", requestedId);
        OledUI::showMessage(String("Assign ") + String(requestedId) + " Denied");
        Storage::logError(ERR_ASSIGN_DENIED);
      }
      break;
    }

    case TX_MODE_ASSIGNED: {
      // Normal PT_PIN transmission handled elsewhere (via pin poller)
      break;
    }
  }

  // RX processing (ACK/NACK reception)
  if (rf69.available()) {
    uint8_t buf[64];
    uint8_t len = sizeof(buf);
    if (!rf69.recv(buf, &len)) return;
    uint8_t type = buf[2];

    if (type == PT_ASSIGN_ACK && txMode == TX_MODE_ASSIGN_REQ) {
      AssignAck* ack = (AssignAck*)buf;
      if (ack->fingerprint == txFingerprint) {
        PeerConfig::setNode(ack->assigned_id, String(ack->node_name));
        OledUI::showMessage("Assigned TX#" + String(ack->assigned_id));
        txMode = TX_MODE_ASSIGNED;
        awaitingAssignResponse = false;
        if (DEBUG_LEVEL & RADIO_DEBUG)
          Serial.printf("[TX] Assigned TX#%d (%s)\n", ack->assigned_id, ack->node_name);
      }
    } else if (type == PT_ASSIGN_NACK && txMode == TX_MODE_ASSIGN_REQ) {
      AssignNack* nack = (AssignNack*)buf;
      if (nack->fingerprint == txFingerprint) {
        const __FlashStringHelper* msg = getAssignErrorMsg(nack->reason);
        Serial.printf("[TX0] Assignment #%d Denied (%s)\n", requestedId, (const char*)msg);
        OledUI::showMessage("Denied: " + String((const char*)msg));
        Storage::logError(ERR_ASSIGN_DENIED);
        txMode = TX_MODE_EPHEMERAL;
        awaitingAssignResponse = false;
      }
    }
  }
}

// ────────────────────────────────
// RX Task
// ────────────────────────────────
void Radio::taskRx(Role role) {
  if (!rf69.available()) return;
  uint8_t buf[64];
  uint8_t len = sizeof(buf);
  if (!rf69.recv(buf, &len)) return;
  uint8_t type = buf[2];

  switch (type) {
    case PT_PIN: {
      Packet* pkt = (Packet*)buf;
      uint8_t peerIndex = pkt->from - 1;
      if (peerIndex >= MAX_TX) break;

      static uint16_t prevPins[MAX_TX] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
      uint16_t newPins = pkt->pins;
      uint16_t changed = prevPins[peerIndex] ^ newPins;
      if (changed) {
        String delta = formatPinDelta(prevPins[peerIndex], newPins);
        if (delta.length() > 0) {
          Serial.printf("RX [%d] %s\n", peerIndex, delta.c_str());
        }
        for (int pin = 0; pin < BTN_COUNT; pin++) {
          if (changed & (1 << pin)) {
            bool newState = (newPins >> pin) & 1;
            if (newState == PRESSED_LEVEL)
              hidHandlePressWithMap(pin, hidMap[peerIndex]);
            else
              hidHandleReleaseWithMap(pin, hidMap[peerIndex]);
          }
        }
      }
      prevPins[peerIndex] = newPins;
      peers[peerIndex].lastRssi = rf69.lastRssi();
      peers[peerIndex].fsm.notePacket();
      break;
    }

    case PT_ADVERTISE: {
      Packet* pkt = (Packet*)buf;
      updateEphemeralTable(pkt->fingerprint, rf69.lastRssi());
      break;
    }

    case PT_ASSIGN_REQUEST: {
      AssignRequest* req = (AssignRequest*)buf;
      handleAssignRequest(*req);
      break;
    }

    default:
      break;
  }
}

// ────────────────────────────────
// Assignment Handling (RX)
// ────────────────────────────────
void Radio::handleAssignRequest(const AssignRequest &req) {
  if (req.requested_id == 0 || req.requested_id > MAX_TX) {
    sendAssignNack(req.fingerprint, ASSIGN_ERR_MALFORM);
    return;
  }

  for (int i = 0; i < MAX_TX; i++) {
    if (nodeTable[i].assigned && nodeTable[i].nodeId == req.requested_id) {
      sendAssignNack(req.fingerprint, ASSIGN_ERR_INUSE);
      return;
    }
  }

  if (!allowAutoNaming && strlen(req.node_name) == 0) {
    sendAssignNack(req.fingerprint, ASSIGN_ERR_NONAME);
    return;
  }

  String assignedName = strlen(req.node_name) > 0
    ? String(req.node_name)
    : String("TX") + String(req.requested_id);

  bool saveOK = Storage::saveConfigForNode(req.requested_id, assignedName);
  if (!saveOK) {
    sendAssignNack(req.fingerprint, ASSIGN_ERR_SAVE);
    return;
  }

  nodeTable[req.requested_id - 1] = {
    .fingerprint = req.fingerprint,
    .nodeId = req.requested_id,
    .nodeName = assignedName,
    .lastRssi = 0,
    .lastSeen = millis(),
    .assigned = true
  };

  AssignAck ack = {};
  ack.fingerprint = req.fingerprint;
  ack.assigned_id = req.requested_id;
  strncpy(ack.node_name, assignedName.c_str(), sizeof(ack.node_name) - 1);
  rf69.send((uint8_t*)&ack, sizeof(ack));

  if (DEBUG_LEVEL & RADIO_DEBUG)
    Serial.printf("[RX] Assigned TX#%d (%s)\n", req.requested_id, ack.node_name);
}

void Radio::sendAssignNack(uint16_t fingerprint, uint8_t reason) {
  AssignNack nack = { .fingerprint = fingerprint, .reason = reason };
  rf69.send((uint8_t*)&nack, sizeof(nack));
  if (DEBUG_LEVEL & RADIO_DEBUG)
    Serial.printf("[RX] NACK sent (fp=0x%04X reason=%d)\n", fingerprint, reason);
}

// ────────────────────────────────
// Ephemeral table update (RX)
// ────────────────────────────────
void Radio::updateEphemeralTable(uint16_t fingerprint, int8_t rssi) {
  bool found = false;
  for (int i = 0; i < MAX_TX; i++) {
    if (nodeTable[i].fingerprint == fingerprint) {
      nodeTable[i].lastSeen = millis();
      nodeTable[i].lastRssi = rssi;
      found = true;
      break;
    }
  }
  if (!found) {
    for (int i = 0; i < MAX_TX; i++) {
      if (!nodeTable[i].assigned) {
        nodeTable[i] = { fingerprint, 0, "Unassigned", rssi, millis(), false };
        break;
      }
    }
  }
}

// ────────────────────────────────
// Common helpers
// ────────────────────────────────
void Radio::sendPacket(Packet &pkt, Role role) {
  pkt.from = PeerConfig::getNodeAddr();
  pkt.to = peerAddress(role);
  pkt.seq = seq++;

  float last_ms, avg_ms, duty_pct;
  computeAirtime(last_ms, avg_ms, duty_pct);
  pkt.air20 = (uint16_t)(last_ms * 10.0f);
  pkt.airtot = (uint16_t)(rollingSum_us / 1000);

  uint32_t start_us = micros();
  rf69.send((uint8_t*)&pkt, sizeof(pkt));
  rf69.waitPacketSent();
  uint32_t dur_us = micros() - start_us;

  lastTxTime = dur_us / 1000.0f;
  recordAirtime(dur_us);
}

void Radio::recordAirtime(uint32_t dur_us) {
  uint32_t now = millis();
  airBuf[head] = { now, dur_us };
  head = (head + 1) % BUF_SIZE;
  if (count < BUF_SIZE) count++;

  rollingSum_us += dur_us;
  uint32_t cutoff = now - 20000;
  int valid = 0;
  uint32_t newSum = 0;

  for (int i = 0; i < count; i++) {
    int idx = (head - 1 - i + BUF_SIZE) % BUF_SIZE;
    if (airBuf[idx].ts >= cutoff) {
      valid++;
      newSum += airBuf[idx].dur;
    } else break;
  }
  count = valid;
  rollingSum_us = newSum;
}

void Radio::computeAirtime(float &last_ms, float &avg_ms, float &duty_pct) {
  if (count == 0) {
    last_ms = avg_ms = duty_pct = 0;
    return;
  }
  uint32_t sum_us = 0;
  for (int i = 0; i < count; i++) {
    int idx = (head - 1 - i + BUF_SIZE) % BUF_SIZE;
    sum_us += airBuf[idx].dur;
  }
  last_ms = airBuf[(head - 1 + BUF_SIZE) % BUF_SIZE].dur / 1000.0f;
  avg_ms = (sum_us / count) / 1000.0f;
  duty_pct = (sum_us / 20000.0f) * 100.0f;
}
