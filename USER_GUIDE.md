# RFM69 Wireless Game Controller — Field Operator Guide

Audience: Non-developer operators deploying and using the wireless controller system in the field.

Hardware: Adafruit Feather RP2040 RFM69 modules (TX and RX), I2C input expander (PCF8575) on TX units, OLED display, host PC for RX via USB.

Firmware: PlatformIO/Arduino build for RP2040 with RFM69 radio modules.

---

## 1) System Overview

- Roles
  - TX (Transmitter): Scans buttons via PCF8575 and sends packets over RFM69.
  - RX (Receiver): Listens for packets and outputs USB HID (keyboard/mouse/gamepad) to a host PC.
- Auto role detection: On boot, devices scan I2C for a PCF8575. If present → TX, otherwise → RX.
- Persistence: Device settings (role, node ID, peer assignments) are saved to on-board flash (LittleFS). Settings persist across reboots.

---

## 2) Quick Start

- RX unit
  - Connect to a host PC via USB-C.
  - Device enumerates as a USB HID device (no special drivers).
  - Display shows RX role and status.

- TX unit
  - Power the device (battery or USB).
  - Display shows TX role and Node ID (TX1..TX4).
  - Press buttons to send inputs to the RX.

- Linking
  - Keep TX and RX within 1–5 meters for initial pairing.
  - TX periodically advertises; RX assigns and acknowledges automatically.
  - Once assigned, TX packets will be accepted by RX and translated to HID.

---

## 3) Node Assignment (TX):
- Long-press button C to cycle node ID.
- System saves new configuration to LittleFS.
- OLED displays confirmation.

---

## 4) Normal Operation

- TX
  - Scans button inputs every ~50 ms.
  - Sends pin updates immediately on change (PT_PIN).
  - Sends periodic heartbeat (PT_HB) to maintain the link.

- RX
  - Receives packets and translates to USB HID actions on the host PC.
  - Supports keyboard, mouse, and gamepad outputs (bindings are firmware-defined).

---

## 5) OLED Display & Status

- Common indicators
  - Role: TX or RX.
  - Node ID: TX1..TX4.
  - Link status: Connected / Searching / Rejoining.
  - Messages during assignment: “ADVERTISE”, “ASSIGN”, “ACK/NACK”.
  - Errors: Brief code or message (also logged to error log in flash).

---

## 6) Power, Reset, and Storage

- Power
  - TX can run from battery or USB.
  - RX requires USB to the host PC.

- Reset
  - Press the reset button briefly to reboot the device.
  - On reboot, role and saved configuration are restored.

- Stored data (LittleFS)
  - /config.json: Global configuration.
  - /nodes/TX<n>.json: Per-node assignment.
  - /errorlog.json: Error/event records.

---

## 7) Troubleshooting

- No input on host PC
  - Verify RX is connected via USB and recognized as HID.
  - Ensure TX and RX are powered and within range.
  - Change TX Node ID (long-press Button C) to match the expected assignment and wait a few seconds.

- TX not linking
  - Move closer to RX; remove obstacles.
  - Power-cycle TX.
  - Ensure only one TX is attempting assignment at a time.

---

## 8) Best Practices

- Keep TX units spaced at least a meter apart during pairing.
- Label TX units with their Node IDs to reduce confusion in the field.
- Maintain line-of-sight where possible; avoid shielding the antennas.
- Power-cycle after major configuration changes to ensure a clean state.

---

## 9) Support Checklist (when reporting issues)

Include:
- Role (TX or RX) and Node ID.
- Steps to reproduce.
- Approximate distance between devices.
- Whether the display shows assignment or rejoin messages.
- Whether the host PC receives any HID events.
- Any error messages shown on OLED.

---