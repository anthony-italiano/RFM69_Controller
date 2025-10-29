#include "Config.h"
#include "Packet.h"
#include "Scheduler.h"
#include "Radio.h"
#include "PCFInput.h"
#include "RejoinFSM.h"
#include "OledUI.h"
#include "Utils.h"
#include "Hid.h"
#include "Storage.h"  // NEW
#include "Peers.h"    // NEW

Role role;
Scheduler scheduler;
Radio radio;
RejoinFSM rejoinFSM;
OledUI oledUI;
PCFInput pcfInput;

void setup() {
  hidBegin();
  delay(2000);
  Serial.begin(115200);
  delay(800);
  Serial.println(F("Booting Rfm69UnifiedController..."));

  // 🔧 Explicitly start I2C before scanning
  Wire.begin();

  if (DEBUG_LEVEL & PCF_DEBUG) {
    for (uint8_t addr = 16; addr < 64; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) {
        Serial.print(F("I2C device at 0x"));
        Serial.println(addr, HEX);
      } else {
        Serial.print(F("."));
      }
      delay(5);
    }
  }

  // Scan I2C to decide role
  bool hasPCF = i2cScanDevice(PCF8575_ADDR);
  role = hasPCF ? Role::TX : Role::RX;
  Serial.print(F("Role: "));
  Serial.println(role == Role::TX ? "TX" : "RX");

  // --- Initialize new modules ---
  delay(200);
  Storage::begin();         // Mount LittleFS
  PeerConfig::begin(role);  // Load node identity (or defaults)
  if (DEBUG_LEVEL & ROLE_DEBUG) {
    Serial.printf("[BOOT] %s node #%d active\n",
                  role == Role::TX ? "TX" : "RX",
                  PeerConfig::getNodeAddr());
  }

  // Init OLED
  oledUI.begin(role);

  // Init radio
  radio.begin(role);

  // Init PCF if TX
  if (role == Role::TX) {
    pcfInput.begin();
  }

  // Setup tasks
  scheduler.addTask("radioRx", 5, [&] {
    radio.task(role);
  });

  scheduler.addTask("rejoin", 30, [&] {
    rejoinFSM.taskRun(radio, role);
  });

  if (role == Role::TX) {
    scheduler.addTask("pcfPoll", 50, [&] {
      pcfInput.taskPoll(radio);
    });
  } else {
    // placeholder for something like
    scheduler.addTask("hidTask", 10, [&] {
      hidTask();
    });
  }
  scheduler.addTask("heartbeat", HEARTBEAT_MS, [&] {
    rejoinFSM.taskHeartbeat(radio, role);
  });

  scheduler.addTask("oled", OLED_INTERVAL, [&] {
    oledUI.taskUpdate(radio, pcfInput, role);
  });

  // --- NEW: periodic background tasks ---
  scheduler.addTask("peerMonitor", 10000, [&] {
    // Future: poll link quality, multi-node health, etc.
    // For now, just verify PeerConfig is valid.
    if (DEBUG_LEVEL & ROLE_DEBUG) {
      Serial.print(F("[MONITOR] Node "));
      Serial.print(PeerConfig::getNodeName());
      Serial.print(F(" ("));
      Serial.print(PeerConfig::getNodeAddr());
      Serial.println(F(") OK"));
    }
  });

  scheduler.addTask("storageFlush", 5000, [&] {
    // Reserved for future log/error persistence
    // (e.g., write queued error data, clean old logs)
  });
}

void loop() {
  scheduler.tick();
}
