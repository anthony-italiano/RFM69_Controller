#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Config.h"

// Config struct persisted in /config.json
struct NodeConfig {
  uint8_t node_addr;   // 1..MAX_TX
  String node_name;    // descriptive name
};

namespace Storage {
  // Core FS management
  bool begin();   // mount LittleFS

  // TX/RX configuration
  bool loadConfig(NodeConfig &cfg);          // load local node config
  bool saveConfig(const NodeConfig &cfg);    // save local node config

  // RX-only: individual node assignments
  bool saveConfigForNode(uint8_t nodeId, const String &name);
  bool loadConfigForNode(uint8_t nodeId, NodeConfig &cfg);

  // Error logging
  void logError(int code);
}
