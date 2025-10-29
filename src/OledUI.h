#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"
#include "Radio.h"
#include "PCFInput.h"

class OledUI {
public:
  Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
  bool dirty = true;  // screen needs refresh

  void begin(Role role);
  void markDirty() { dirty = true; }
  void taskUpdate(Radio &radio, PCFInput &pcf, Role role);

  // ────────────────────────────────
  // Public utility: show transient message
  // ────────────────────────────────
  static void showMessage(const String &msg, uint16_t durationMs = 3000);

private:
  // ────────────────────────────────
  // Internal state for button handling
  // ────────────────────────────────
  static constexpr uint16_t LONG_PRESS_MS = 2000;
  unsigned long cPressStart = 0;
  bool cHeld = false;
  bool awaitingAck = false;

  // ────────────────────────────────
  // Message handling
  // ────────────────────────────────
  static bool showingMessage;
  static unsigned long messageStart;
  static uint16_t messageDuration;
  static String messageText;

  // ────────────────────────────────
  // Internal helpers
  // ────────────────────────────────
  void drawSavedScreen();
  bool buttonPressed(uint8_t pin);
  void renderMessage();
};
