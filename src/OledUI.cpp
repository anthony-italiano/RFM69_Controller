#include "OledUI.h"
#include "Config.h"
#include "Utils.h"
#include "RejoinFSM.h"
#include "Peers.h"

extern RejoinFSM rejoinFSM;

// ────────────────────────────────
// Static message state
// ────────────────────────────────
bool OledUI::showingMessage = false;
unsigned long OledUI::messageStart = 0;
uint16_t OledUI::messageDuration = 0;
String OledUI::messageText;

// ────────────────────────────────
// Local constants for button pins
// ────────────────────────────────
#define BTN_A 9
#define BTN_B 6
#define BTN_C 5

// ────────────────────────────────
// Small debounce helper
// ────────────────────────────────
bool OledUI::buttonPressed(uint8_t pin) {
  static uint32_t lastDebounce = 0;
  bool pressed = !digitalRead(pin);
  if (pressed && millis() - lastDebounce > 50) {
    lastDebounce = millis();
    return true;
  }
  return false;
}

// ────────────────────────────────
// OLED initialization
// ────────────────────────────────
void OledUI::begin(Role role) {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    if (DEBUG_LEVEL & OLED_DEBUG)
      Serial.println(F("[OLED] init failed"));
    for (;;)
      ;  // Halt
  }

  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_C, INPUT_PULLUP);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("OLED Online"));
  display.setCursor(0, 10);
  display.print(F("Role: "));
  display.println(role == Role::TX ? "TX" : "RX");
  display.display();

  dirty = false;

  if (DEBUG_LEVEL & OLED_DEBUG)
    Serial.println(F("[OLED] init OK + first frame"));
}

// ────────────────────────────────
// Draw the “Saved!” confirmation screen
// ────────────────────────────────
void OledUI::drawSavedScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Saved!"));

  display.setCursor(0, 12);
  display.print(F("Node "));
  display.print(PeerConfig::getNodeAddr());

  display.setCursor(0, 22);
  display.print(PeerConfig::getNodeName());
  display.display();

  if (DEBUG_LEVEL & OLED_DEBUG)
    Serial.println(F("[OLED] Saved! screen shown"));
}

// ────────────────────────────────
// Unified transient message display
// ────────────────────────────────
void OledUI::showMessage(const String &msg, uint16_t durationMs) {
  // Static instance pattern to ensure valid I2C object
  static OledUI *instance = nullptr;
  if (!instance) instance = new OledUI();

  showingMessage = true;
  messageText = msg;
  messageStart = millis();
  messageDuration = durationMs;

  instance->renderMessage();

  if (DEBUG_LEVEL & OLED_DEBUG) {
    Serial.print(F("[OLED] showMessage: "));
    Serial.println(msg);
  }
}

// ────────────────────────────────
// Render the current transient message
// ────────────────────────────────
void OledUI::renderMessage() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(messageText);
  display.display();
}

// ────────────────────────────────
// Main update / interaction task
// ────────────────────────────────
void OledUI::taskUpdate(Radio &radio, PCFInput &pcf, Role role) {
  // Handle global message timeout
  if (showingMessage) {
    if (millis() - messageStart > messageDuration) {
      showingMessage = false;
      dirty = true;
    } else {
      return;  // pause UI updates during message
    }
  }

  // Handle TX-specific input
  if (role == Role::TX) {
    // Ignore input during confirmation display
    if (awaitingAck) {
      if (buttonPressed(BTN_A) || buttonPressed(BTN_B) || buttonPressed(BTN_C)) {
        awaitingAck = false;
        dirty = true;  // trigger screen restore
        if (DEBUG_LEVEL & OLED_DEBUG)
          Serial.println(F("[OLED] Acknowledgment received, restoring UI"));
      } else {
        return;  // keep showing Saved! screen
      }
    }

    bool cPressed = !digitalRead(BTN_C);

    // ---- Long press detection ----
    if (cPressed && !cHeld && cPressStart == 0) {
      cPressStart = millis();
    } else if (cPressed && !cHeld && (millis() - cPressStart) > LONG_PRESS_MS) {
      // Long press detected — show preview of next node
      cHeld = true;

      uint8_t next = PeerConfig::getNodeAddr() + 1;
      if (next > MAX_TX) next = 1;
      String nextName = String("TX") + String(next);

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print(F("Next: "));
      display.println(nextName);
      display.setCursor(0, 22);
      display.print(F("Release to save"));
      display.display();

      if (DEBUG_LEVEL & OLED_DEBUG) {
        Serial.print(F("[OLED] Long press detected, preview next node: "));
        Serial.println(nextName);
      }
    } else if (!cPressed && cHeld) {
      // Released after long hold — commit new node
      cHeld = false;
      cPressStart = 0;

      PeerConfig::nextNode();  // handles wrapping and saving
      awaitingAck = true;
      drawSavedScreen();
      dirty = false;

      if (DEBUG_LEVEL & OLED_DEBUG)
        Serial.println(F("[OLED] Node assignment saved"));
      return;
    } else if (!cPressed && cPressStart > 0 && !cHeld) {
      // Short press (cancel)
      cPressStart = 0;
    }
  }

  if (!dirty) return;

  // ───────── Normal display rendering ─────────
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  switch (role) {
    case Role::TX:
      display.setCursor(0, 0);
      display.print(F("Role:TX LINK:"));
      display.print(rejoinFSM.link == LinkState::UP ? "U" : "D");

      display.setCursor(0, 10);
      display.print(F("Seq:"));
      display.print(radio.seq);

      display.setCursor(0, 20);
      {
        String delta = formatPinDelta(pcf.prev, pcf.pinsState);
        if (delta.length() > 0) {
          display.print(F("I "));
          display.print(delta);
        }
      }

      display.setCursor(70, 0);
      display.print(F("ID"));
      display.print(PeerConfig::getNodeAddr());
      break;

    case Role::RX:
      for (int i = 0; i < MAX_TX; i++) {
        display.setCursor(0, i * 10);
        display.print(F("TX"));
        display.print(i + 1);
        display.print(F(":"));
        if (peers[i].fsm.link == LinkState::UP) {
          display.print(peers[i].lastRssi);
        } else {
          display.print(F("--"));
        }
      }
      break;
  }

  display.display();
  dirty = false;

  if (DEBUG_LEVEL & OLED_DEBUG)
    Serial.println(F("[OLED] refreshed"));
}
