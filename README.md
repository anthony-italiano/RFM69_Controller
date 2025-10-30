# RFM69 Wireless Game Controller (Adafruit Feather RP2040 RFM69)

Arduino/PlatformIO firmware for a modular wireless game controller built around RFM69 radios and the Adafruit Feather RP2040 RFM69. The system supports TX (input) nodes and an RX (USB HID) receiver, with persistent config and an OLED UI.

- Board: Adafruit Feather RP2040 RFM69 (RP2040 + RFM69HCW)
- Radio: RFM69 (e.g., 915 MHz)
- Framework: Arduino (Earle Philhower core)
- Build: PlatformIO (VS Code)

## Features
- Auto role detection (TX/RX) via I2C scan for PCF8575
- Encrypted RFM69 packet protocol (advertise/assign/heartbeat/pin updates)
- USB HID on RX (keyboard/mouse/gamepad)
- LittleFS-based config and error log
- OLED UI for status/config
- Cooperative scheduler

## Project Structure
- platformio.ini (env config for Adafruit Feather RP2040 RFM69)
- Source files (sketch + modules: Radio, PCFInput, Hid, OledUI, Storage, Peers, Scheduler, RejoinFSM, Packet, Utils)
- .pio/ (build output, ignored)
- .vscode/ (workspace settings)

## Build (PlatformIO)
VS Code:
- Open folder in VS Code
- PlatformIO: Build (checkmark)

CLI (PowerShell):
```powershell
pio run
```

platformio.ini (key settings):
```ini
[platformio]
default_envs = adafruit_feather_rfm69

[env:adafruit_feather_rfm69]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = adafruit_feather_rfm
framework = arduino
board_build.core = earlephilhower
board_build.mcu = rp2040
upload_protocol = picotool
```

## Upload (picotool)
Windows driver (once):
1) Put board in BOOTSEL (hold BOOTSEL while plugging USB)
2) Use Zadig → Options → List All Devices
3) Install WinUSB for both:
   - RP2 BOOT (Interface 0)
   - RP2 BOOT (Interface 1)

Upload:
- Enter BOOTSEL (hold BOOTSEL + press RESET)
- PlatformIO: Upload (arrow) or:
```powershell
pio run -t upload -e adafruit_feather_rfm69
```

Troubleshooting:
- “No accessible RPxxxx devices”: confirm WinUSB on both interfaces in Zadig, unplug/replug in BOOTSEL.
- Fallback: drag-and-drop .uf2 from .pio/build/<env>/ to the mounted drive (if using mass storage driver).

## Runtime Components
- Storage (LittleFS): /config.json, /nodes/TX<n>.json, /errorlog.json
- Scheduler: radio/input/oled/heartbeat tasks
- Radio protocol (Packet.h): PT_ADVERTISE, PT_ASSIGN_REQUEST/ACK/NACK, PT_PIN, PT_HB
- HID mappings (Config): keyboard/mouse/gamepad bindings per TX

## Development Tips
- Verbose build: `pio run -v`
- Clean: `pio run -t clean`
- Serial monitor: `pio device monitor`
- If IntelliSense errors: let PlatformIO manage includes; avoid mixing Arduino IDE build flow.

## License
Add your license here (e.g., MIT). Include third‑party licenses as needed.

## Acknowledgments
- Earle Philhower Arduino-Pico core
- Adafruit Feather RP2040 RFM69
- RP2040 + RFM69 community resources
