#include "Storage.h"
#include "Config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Local forward declaration
static void rotateErrorLog(JsonArray &errors, int code, const char *msg, bool sticky);

// ---------------------------------------------------------------------------
// Initialize LittleFS
// ---------------------------------------------------------------------------
bool Storage::begin() {
  if (!LittleFS.begin()) {
    if (DEBUG_LEVEL & FS_DEBUG) Serial.println(F("[FS] mount failed, formatting..."));

    LittleFS.format();
    if (!LittleFS.begin()) {
      if (DEBUG_LEVEL & FS_DEBUG) Serial.println(F("[FS] re-mount failed"));
      Storage::logError(ERR_FS_MOUNT_FAIL);
      return false;
    }
    if (DEBUG_LEVEL & FS_DEBUG) Serial.println(F("[FS] re-mount succeeded after format"));
  } else {
    if (DEBUG_LEVEL & FS_DEBUG) Serial.println(F("[FS] mount OK"));
  }
  return true;
}

// ---------------------------------------------------------------------------
// Load config.json → NodeConfig struct
// ---------------------------------------------------------------------------
bool Storage::loadConfig(NodeConfig &cfg) {
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    if (DEBUG_LEVEL & FS_DEBUG) Serial.println(F("[FS] config.json missing"));
    Storage::logError(ERR_CONFIG_MISSING);
    cfg.node_addr = DEFAULT_NODE_ADDR;
    cfg.node_name = DEFAULT_NODE_NAME;
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    if (DEBUG_LEVEL & FS_DEBUG) {
      Serial.print(F("[FS] JSON parse error: "));
      Serial.println(err.f_str());
    }
    Storage::logError(ERR_JSON_PARSE);
    return false;
  }

  cfg.node_addr = doc["node_addr"] | DEFAULT_NODE_ADDR;
  cfg.node_name = String(doc["node_name"] | DEFAULT_NODE_NAME);

  if (DEBUG_LEVEL & FS_DEBUG) {
    Serial.printf("[FS] Loaded node addr=%d name=%s\n", cfg.node_addr, cfg.node_name.c_str());
  }
  return true;
}

// ---------------------------------------------------------------------------
// Save config.json ← NodeConfig struct
// ---------------------------------------------------------------------------
bool Storage::saveConfig(const NodeConfig &cfg) {
  File f = LittleFS.open("/config.json", "w");
  if (!f) {
    if (DEBUG_LEVEL & FS_DEBUG) Serial.println(F("[FS] save open failed"));
    Storage::logError(ERR_SAVE_FAIL);
    return false;
  }

  JsonDocument doc;
  doc["node_addr"] = cfg.node_addr;
  doc["node_name"] = cfg.node_name;

  if (serializeJson(doc, f) == 0) {
    if (DEBUG_LEVEL & FS_DEBUG) Serial.println(F("[FS] save write fail"));
    Storage::logError(ERR_SAVE_FAIL);
    f.close();
    return false;
  }

  f.close();
  if (DEBUG_LEVEL & FS_DEBUG)
    Serial.printf("[FS] Saved node %d -> %s\n", cfg.node_addr, cfg.node_name.c_str());
  return true;
}

// ---------------------------------------------------------------------------
// RX-ONLY: Save assigned TX node configuration
// ---------------------------------------------------------------------------
// Saves to: /nodes/TX<n>.json
// Example path: /nodes/TX3.json
// ---------------------------------------------------------------------------
bool Storage::saveConfigForNode(uint8_t nodeId, const String &name) {
  if (nodeId == 0 || nodeId > MAX_TX) return false;

  // Ensure directory exists
  if (!LittleFS.exists("/nodes")) {
    LittleFS.mkdir("/nodes");
  }

  String path = String("/nodes/TX") + String(nodeId) + ".json";
  File f = LittleFS.open(path, "w");
  if (!f) {
    if (DEBUG_LEVEL & FS_DEBUG)
      Serial.printf("[FS] saveConfigForNode: open fail (%s)\n", path.c_str());
    Storage::logError(ERR_SAVE_FAIL);
    return false;
  }

  JsonDocument doc;
  doc["node_addr"] = nodeId;
  doc["node_name"] = name;

  if (serializeJson(doc, f) == 0) {
    if (DEBUG_LEVEL & FS_DEBUG)
      Serial.printf("[FS] saveConfigForNode: write fail (%s)\n", path.c_str());
    f.close();
    Storage::logError(ERR_SAVE_FAIL);
    return false;
  }

  f.close();
  if (DEBUG_LEVEL & FS_DEBUG)
    Serial.printf("[FS] Saved TX#%d -> %s\n", nodeId, name.c_str());
  return true;
}

// ---------------------------------------------------------------------------
// RX-ONLY: Load a TX node configuration (used on RX boot)
// ---------------------------------------------------------------------------
bool Storage::loadConfigForNode(uint8_t nodeId, NodeConfig &cfg) {
  if (nodeId == 0 || nodeId > MAX_TX) return false;

  String path = String("/nodes/TX") + String(nodeId) + ".json";
  File f = LittleFS.open(path, "r");
  if (!f) {
    if (DEBUG_LEVEL & FS_DEBUG)
      Serial.printf("[FS] loadConfigForNode: missing %s\n", path.c_str());
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    if (DEBUG_LEVEL & FS_DEBUG)
      Serial.printf("[FS] loadConfigForNode: parse error (%s)\n", path.c_str());
    return false;
  }

  cfg.node_addr = doc["node_addr"] | nodeId;
  String defaultName = String("TX") + String(nodeId);
  cfg.node_name = String(doc["node_name"] | defaultName.c_str());

  if (DEBUG_LEVEL & FS_DEBUG)
    Serial.printf("[FS] Loaded TX#%d -> %s\n", cfg.node_addr, cfg.node_name.c_str());
  return true;
}

// ---------------------------------------------------------------------------
// Append to /errorlog.json with sticky retention policy
// ---------------------------------------------------------------------------
void Storage::logError(int code) {
  const char *msg = lookupErrorMsg(code);
  bool sticky = false;

  for (int i = 0; i < ERROR_DEF_COUNT; i++) {
    if (ERROR_DEFS[i].code == code) {
      sticky = ERROR_DEFS[i].sticky;
      break;
    }
  }

  File f = LittleFS.open("/errorlog.json", "r");
  JsonDocument doc;
  if (f) {
    deserializeJson(doc, f);
    f.close();
  }

  JsonArray errors = doc.to<JsonArray>();
  rotateErrorLog(errors, code, msg, sticky);

  f = LittleFS.open("/errorlog.json", "w");
  if (f) {
    serializeJson(errors, f);
    f.close();
  }

  if (DEBUG_LEVEL & FS_DEBUG) {
    Serial.printf("[ERROR %d] %s\n", code, msg);
  }
}

// ---------------------------------------------------------------------------
// Helper: rotate error log according to sticky policy
// ---------------------------------------------------------------------------
static void rotateErrorLog(JsonArray &errors, int code, const char *msg, bool sticky) {
  const size_t MAX_ERRORS = 10;

  if (errors.size() >= MAX_ERRORS) {
    bool evicted = false;
    for (size_t i = 0; i < errors.size(); i++) {
      if (!errors[i]["sticky"].as<bool>()) {
        errors.remove(i);
        evicted = true;
        break;
      }
    }
    if (!evicted && sticky) errors.remove(0);
    else if (!evicted) return;
  }

  JsonObject e = errors.add<JsonObject>();
  e["code"] = code;
  e["msg"] = msg;
  e["sticky"] = sticky;
  e["ts"] = millis();
}
