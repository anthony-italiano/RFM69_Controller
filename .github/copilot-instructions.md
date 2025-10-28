# RFM69 Wireless Game Controller - AI Agent Instructions

## Project Overview
This is an Arduino-based wireless game controller project that uses RFM69 radio modules for communication. The system consists of transmitter (TX) and receiver (RX) nodes, where:
- TX nodes act as input controllers (buttons/gamepads)
- RX nodes receive input and translate it to USB HID commands

## Core Architecture

### 1. Role Detection & Initialization
- System auto-detects role (TX/RX) by scanning I2C for PCF8575 input device
- Entry point: `RFM69UnifiedModular_0_27_Assignment.ino`
- Component initialization sequence in `setup()`:
  1. HID interface
  2. I2C bus
  3. Storage system (LittleFS)
  4. Peer configuration
  5. OLED display
  6. Radio module
  7. Input device (TX only)

### 2. Key Components
- `Radio` (`Radio.h/cpp`): Manages RFM69 radio communication and packet handling
- `PCFInput` (`PCFInput.h/cpp`): Handles button input scanning on TX nodes
- `Hid` (`Hid.h/cpp`): USB HID interface for keyboard/mouse/gamepad emulation
- `OledUI` (`OledUI.h/cpp`): Display and user interface management
- `Storage` (`Storage.h/cpp`): Persistent configuration using LittleFS
- `Peers` (`Peers.h/cpp`): Peer discovery and management
- `Scheduler` (`Scheduler.h/cpp`): Task scheduling system

### 3. Communication Protocol
- Packet structure defined in `Packet.h`
- Key packet types:
  - `PT_PIN`: Button state updates
  - `PT_HB`: Heartbeat
  - `PT_ADVERTISE`: Node discovery
  - `PT_ASSIGN_REQUEST/ACK/NACK`: Node assignment protocol

## Critical Patterns

### 1. State Management
- Each TX node has a unique ID (1-4) and name
- Link state managed through `RejoinFSM`
- Configuration persisted in LittleFS:
  - `/config.json`: Node configuration
  - `/nodes/TX<n>.json`: TX node assignments

### 2. HID Mapping
- Defined in `Config.cpp`
- Structure: `HidBinding[MAX_TX][BTN_COUNT]`
- Supports multiple input types:
  - Keyboard (with modifiers)
  - Mouse (buttons and movement)
  - Gamepad (buttons and axes)

### 3. Task Scheduling
- Cooperative multitasking via `Scheduler`
- Key intervals defined in `config.h`
- Critical tasks:
  - Radio communication (5ms)
  - Input polling (50ms)
  - Display updates (150ms)
  - Heartbeat monitoring (10000ms)

## Important Files
- `config.h`: Core configuration and constants
- `Radio.h/cpp`: Radio communication core
- `Hid.h/cpp`: USB HID implementation
- `PCFInput.h/cpp`: Input handling
- `OledUI.h/cpp`: User interface

## Common Operations
1. Node Assignment (TX):
   - Long-press button C to cycle node ID
   - System saves new configuration to LittleFS
   - OLED displays confirmation

2. Input Processing:
   - TX: PCF8575 polls every 50ms
   - Changes trigger immediate radio packets
   - RX: Translates to USB HID commands

## Project Conventions
1. Debug Flags:
   ```cpp
   #define OLED_DEBUG  0b00000001
   #define PCF_DEBUG   0b00000010
   #define RADIO_DEBUG 0b00000100
   #define FSM_DEBUG   0b00001000
   // etc.
   ```

2. Error Handling:
   - Error codes defined in `config.h`
   - Errors logged to `/errorlog.json`
   - Supports sticky vs non-sticky errors

3. Radio Protocol:
   - 915 MHz frequency
   - Encrypted communication
   - Packet size: 14 bytes