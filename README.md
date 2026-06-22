# 🔐 ESP32 ATECC608A Crypto Wallet Prototype

> Work-in-progress ESP32-based crypto wallet prototype with TFT display, serial-terminal control, partial joystick integration, PIN-based access control, EEPROM persistence, Wi-Fi configuration, and planned ATECC608A secure element support.

[![License](https://img.shields.io/badge/License-PolyForm%20Noncommercial-blue.svg)](LICENSE)
![Status](https://img.shields.io/badge/Status-Work%20in%20Progress-orange)
![Platform](https://img.shields.io/badge/Platform-ESP32-green)
![Secure Element](https://img.shields.io/badge/Secure%20Element-ATECC608A%20Planned-lightgrey)
![Language](https://img.shields.io/badge/Language-Arduino%20C%2B%2B-blue)

## 🚧 Project Status

**Status: Work in Progress**

This project is not finished and must not be used as a real cryptocurrency wallet.

The current implementation is a functional embedded prototype focused on:

- Modular embedded firmware architecture
- TFT-based wallet user interface
- Serial terminal control through a Python helper
- Partial joystick input integration
- PIN-based access control
- EEPROM-backed prototype persistence
- Wi-Fi configuration and connectivity testing
- Future ATECC608A secure element integration

The repository name includes `ATECC608A` because the final goal is to integrate an ATECC608A secure element for safer key storage and cryptographic operations.

At the current stage, the ATECC608A module is part of the target hardware design, but secure element integration is not yet implemented in the wallet core.

## ⚠️ Important Security Warning

This project is for educational, experimental, and prototyping purposes only.

Do not use this firmware to store real cryptocurrency funds.

Current limitations include:

- Private key material is currently handled by the ESP32 firmware.
- Key material is currently persisted using ESP32 EEPROM emulation.
- Address generation is prototype-level and not a production blockchain derivation flow.
- ATECC608A secure element integration is not yet completed.
- No production-grade signing flow is implemented yet.
- No audited cryptographic design is provided.
- No tamper resistance is implemented.
- No secure backup or recovery mechanism is implemented.

This project should be treated as an embedded security and wallet UI prototype, not as a production wallet.

## 📸 Prototype Media

### Current Breadboard Prototype

![Current breadboard prototype](docs/media/breadboard-current.jpg)

The current prototype uses a NodeMCU-ESP32 / ESP32 DEVKITV1 board, an ST7735S SPI TFT display, an HW-504 joystick module, and a WCMCU ATECC608A secure element module on a breadboard.

### Current Functionality Demo

[Current functionality demo](docs/media/current-demo.mp4)

The current demo video shows the prototype being controlled through the Python serial terminal helper using keyboard arrow keys.

At this stage:

- The Python terminal helper is experimental.
- The terminal display mirror is not perfect yet.
- The video focuses on keyboard-arrow navigation through the Python tool.
- Physical joystick support exists in the firmware and has a dedicated test sketch, but the joystick control mode is still being tested and is not the main demo path yet.

Suggested demo contents:

- Device powered on
- TFT menu displayed
- Python terminal helper running
- Keyboard arrow navigation
- PIN entry
- Wallet unlock
- Address screen
- Info screen
- Wi-Fi menu or Wi-Fi test screen

## 📌 Overview

This project implements a prototype hardware crypto wallet on an ESP32.

The device provides a small standalone interface using:

- An ST7735S TFT display
- Serial terminal input
- Partial HW-504 joystick input
- PIN entry
- Menu navigation
- Wallet lock/unlock state
- Address display
- Private key export for debugging
- Wi-Fi configuration storage
- Wi-Fi connectivity testing
- EEPROM reset helper tooling

The strongest part of the current project is the software architecture: the firmware is divided into independent modules for presentation, input handling, state management, and wallet/domain logic.

This makes the project easier to extend, debug, test, and eventually migrate from EEPROM-based prototype storage to ATECC608A-backed secure key handling.

## ✨ Main Features

- ESP32 firmware written in Arduino/C++.
- ST7735S TFT display support.
- Serial terminal navigation support.
- Experimental joystick navigation support.
- Input abstraction through an input facade.
- Menu-based wallet interface.
- PIN creation and unlock flow.
- Wallet lock/unlock state management.
- Address display.
- Private key export for development/debugging.
- PIN change flow.
- Factory reset flow.
- Wi-Fi credential entry through the device interface.
- Saved Wi-Fi configuration in EEPROM.
- Wi-Fi connectivity test.
- Serial mirroring of display screens.
- Python terminal client for host-side interaction.
- Helper sketches for LCD testing, joystick testing, and EEPROM clearing.
- Planned ATECC608A secure element integration.

## 🧭 Current Menu

The current firmware exposes the following main menu:

```text
Unlock / Lock
Address
Balance
Export
Change PIN
Info
Wi-Fi
Reset
```

The Wi-Fi submenu currently includes:

```text
Test Internet
Saved WiFi
New WiFi
Back
```

Supported Wi-Fi security options:

```text
WPA2-PSK
WPA3
WPA/WPA2
WEP
Open
```

## 🔩 Hardware Components

Current prototype components:

| Component | Exact module / marking | Purpose |
|---|---|---|
| ESP32 development board | `NodeMCU-ESP32` / `ESP32 DEVKITV1` / `HW-463C V0.0.6` | Main development board |
| ESP32 module | `ESP-WROOM-32` | Main microcontroller module on the development board |
| TFT display | `M180-128160-RGB-8-V1.0` | Wallet graphical interface |
| Display driver IC | `ST7735S` | TFT LCD controller |
| Display interface | `4LINE SPI` | Communication interface used by the display |
| Joystick module | `HW-504` | Physical navigation and selection input |
| Secure element module | `WCMCU ATECC608A` | Planned secure key storage and cryptographic operations |
| Breadboard | Standard prototyping breadboard | Temporary hardware assembly |
| Jumper wires | Dupont jumper wires | Hardware connections between modules |

The current prototype is built on a breadboard using an ESP32 development board, an ST7735S-based TFT display, an HW-504 joystick module, and a WCMCU ATECC608A secure element module.

The ATECC608A module is physically part of the target hardware design, but secure element integration is still planned and not yet fully implemented in the wallet core.

## 🔌 Current Wiring

Current firmware pin configuration:

| Module | Signal | ESP32 GPIO | Notes |
|---|---|---:|---|
| ST7735S TFT display | CS | GPIO 5 | Chip select |
| ST7735S TFT display | DC | GPIO 2 | Data/command selection |
| ST7735S TFT display | RST | GPIO 4 | Display reset |
| HW-504 joystick | Y axis | GPIO 13 | Analog vertical navigation |
| HW-504 joystick | Button | GPIO 15 | Selection/confirmation button |

Current definitions in the main firmware:

```cpp
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define JOY_Y    13
#define JOY_BTN  15
```

The TFT display module is marked as:

```text
M180-128160-RGB-8-V1.0
INTERFACE: 4LINE SPI
DRIVER IC: ST7735S
```

The ESP32 development board is identified as:

```text
NodeMCU-ESP32
ESP32 DEVKITV1
HW-463C V0.0.6
ESP-WROOM-32
```

The joystick module is identified as:

```text
HW-504
```

The secure element module is identified as:

```text
WCMCU ATECC608A
```

> Note: wiring should be validated against the exact board revision and module pinout before reproducing the prototype, because ESP32 development boards and TFT display modules may expose pins differently depending on the manufacturer.

## 🧱 Professional Software Architecture

The firmware is designed using a modular, layered architecture instead of placing all logic directly inside the main Arduino sketch.

The goal is to separate responsibilities clearly:

- UI rendering should not handle wallet logic.
- Input parsing should not depend on display rendering.
- Wallet state should not be spread across unrelated code.
- Storage and key handling should be isolated behind a core module.
- Hardware input can evolve without rewriting the application flow.
- EEPROM-based prototype logic can later be replaced by ATECC608A-backed logic with less impact on the UI and input layers.

### High-Level Architecture

```text
crypto-wallet-dual.ino
│
├── Presentation Layer
│   └── DisplayManager
│       ├── ST7735 display rendering
│       ├── Menu rendering
│       ├── PIN entry screen
│       ├── Wi-Fi entry screens
│       ├── Info/address/private key screens
│       └── Serial screen mirroring
│
├── Input Layer
│   ├── InputManager
│   │   ├── Serial terminal input
│   │   ├── Keyboard arrow parsing
│   │   ├── ENTER/BACKSPACE/digit parsing
│   │   ├── Joystick analog input
│   │   ├── Joystick button input
│   │   └── Debounce/cooldown logic
│   │
│   └── InputFacade
│       ├── Unified input interface
│       ├── Active input mode tracking
│       └── Input source abstraction
│
├── Application State Layer
│   └── StateManager
│       ├── Application state machine
│       ├── Menu selection state
│       ├── Wallet lock state
│       ├── PIN entry state
│       └── Message/info state
│
├── Domain/Core Layer
│   └── WalletCore
│       ├── PIN persistence
│       ├── Wallet lock/unlock logic
│       ├── Prototype key generation
│       ├── Prototype address generation
│       ├── EEPROM storage
│       ├── Wi-Fi credential storage
│       └── Factory reset
│
├── Host Tooling
│   └── wallet_terminal.py
│       ├── Keyboard input forwarding
│       ├── Arrow key forwarding
│       └── Serial output display
│
└── Hardware Test Tools
    ├── test-lcd.ino
    ├── joystick-test.ino
    └── clear-eeprom.ino
```

### Design Principles

This project follows several software design principles that are useful in embedded development:

| Principle | How it appears in this project |
|---|---|
| Separation of concerns | Display, input, state, and wallet logic are separated into different modules. |
| Single responsibility | Each manager class owns one main part of the system. |
| Facade pattern | `InputFacade` provides a simplified interface over the input subsystem. |
| State-driven UI | `StateManager` stores current and previous state so the display can react to changes. |
| Incremental rendering | `DisplayManager` can update only changed UI elements instead of redrawing everything every time. |
| Hardware abstraction | Input and display logic are isolated from wallet/domain logic. |
| Replaceable storage design | `WalletCore` currently uses EEPROM, but it is the future integration point for ATECC608A-backed storage/signing. |
| Testability | LCD, joystick, and EEPROM can be tested independently through helper sketches. |

## 🧠 Architectural Highlights

### 🖥️ DisplayManager

`DisplayManager` owns the presentation layer.

Responsibilities:

- Initialize the ST7735 display.
- Render the main menu.
- Update only changed menu items when possible.
- Render wallet status and active input mode.
- Display PIN entry.
- Display address, private key, and info screens.
- Display Wi-Fi text entry and Wi-Fi test screens.
- Mirror screen output to Serial.

This keeps rendering logic separate from wallet logic and input handling.

### 🎮 InputManager

`InputManager` owns low-level input handling.

Responsibilities:

- Parse serial terminal input.
- Parse keyboard arrow escape sequences.
- Handle ENTER, BACKSPACE, and digit input.
- Read joystick analog values.
- Read joystick button state.
- Apply debounce and cooldown logic.
- Prioritize terminal input when multiple input events are available.

The joystick path is present but still under testing. The current demo focuses on the serial terminal path.

### 🔀 InputFacade

`InputFacade` provides a simplified interface over the input subsystem.

Responsibilities:

- Expose a unified input API to the application.
- Track the active input mode.
- Forward PIN input mode changes.
- Keep the rest of the firmware independent from whether input comes from terminal or joystick.

This makes it easier to improve joystick behavior later without rewriting the application flow.

### 🧩 StateManager

`StateManager` owns application state.

Current state categories include:

- Main menu
- PIN entry
- Message screen
- Address display
- Private key display
- Info screen
- Wi-Fi test
- Wi-Fi menu
- Wi-Fi string entry
- Wi-Fi security selection

It also tracks:

- Current and previous application state
- Current and previous menu item
- Wallet lock state
- PIN input array
- PIN cursor position
- PIN digit visibility
- Active input mode
- Current message

This gives the firmware a cleaner state-machine style structure instead of scattering state variables across the main sketch.

### 🔐 WalletCore

`WalletCore` owns the wallet/domain layer.

Current responsibilities:

- Initialize EEPROM.
- Load and save PIN.
- Lock and unlock wallet.
- Generate prototype private key material.
- Save and load private key material.
- Generate a prototype address.
- Export private key as hexadecimal string.
- Store Wi-Fi credentials.
- Clear Wi-Fi configuration.
- Factory reset EEPROM-backed state.
- Change PIN.

The final objective is to migrate sensitive key operations from ESP32 firmware/EEPROM into the ATECC608A secure element.

## 💾 Current Storage Layout

The current prototype uses ESP32 EEPROM emulation.

Current approximate layout:

| EEPROM Range | Purpose |
|---|---|
| `0-1` | PIN |
| `32-63` | Prototype private key |
| `64` | Wi-Fi saved flag |
| `65` | Wi-Fi security type |
| `66` | SSID length |
| `67-98` | SSID |
| `99` | Wi-Fi password length |
| `100-127` | Wi-Fi password |

This layout is for prototyping only and should not be considered secure storage.

## 🧪 Helper Sketches

This repository includes helper sketches used during hardware bring-up and testing.

These are intentionally kept separate from the main firmware because each one validates a specific hardware or storage component.

### `test-lcd.ino`

Purpose:

- Initializes the ST7735S TFT display.
- Runs basic color tests.
- Prints test text to confirm display operation.

Useful for validating:

- Display wiring
- SPI communication
- Screen initialization
- Text rendering

### `joystick-test.ino`

Purpose:

- Reads HW-504 joystick analog values.
- Displays joystick state on the TFT.
- Prints analog readings to Serial.

Useful for validating:

- Joystick wiring
- ADC readings
- Button state
- Up/down thresholds

### `clear-eeprom.ino`

Purpose:

- Initializes EEPROM.
- Writes `0xFF` across the EEPROM range.
- Commits changes.

Useful for resetting the prototype state during development.

## 💻 Host Terminal Client

The repository includes an experimental Python helper:

```text
wallet_terminal.py
```

This tool provides a host-side terminal interface for controlling the ESP32 over Serial.

Features:

- Opens the serial port.
- Enters raw terminal mode.
- Forwards arrow keys.
- Forwards ENTER and BACKSPACE.
- Forwards digit input.
- Displays serial output from the device.
- Uses an alternate terminal screen.
- Press `q` to quit.

Current limitations:

- The terminal display mirror is not perfect yet.
- Some redraw behavior may not exactly match the TFT output.
- The tool is currently most useful as an input bridge for keyboard-arrow navigation.
- The current demo video uses this tool rather than the physical joystick.

Default configuration:

```python
WalletTerminal(port='/dev/ttyUSB0', baudrate=115200)
```

Example usage:

```bash
python3 wallet_terminal.py
```

If your ESP32 appears on a different serial port, edit the port value in the script.

## 📦 Software Requirements

Arduino/C++ firmware:

- Arduino IDE or compatible build environment
- ESP32 Arduino core
- `Adafruit_GFX`
- `Adafruit_ST7735`
- `SPI`
- `EEPROM`
- `WiFi`
- `Crypto`
- `SHA256`

Python terminal helper:

- Python 3
- `pyserial`

Install Python dependency:

```bash
pip install pyserial
```

## 📁 Recommended Repository Structure

Recommended professional structure:

```text
esp32-atecc608a-crypto-wallet/
├── README.md
├── LICENSE
├── docs/
│   └── media/
│       ├── breadboard-current.jpg
│       └── current-demo.mp4
├── firmware/
│   └── crypto-wallet-dual/
│       ├── crypto-wallet-dual.ino
│       ├── DisplayManager.cpp
│       ├── DisplayManager.h
│       ├── InputFacade.cpp
│       ├── InputFacade.h
│       ├── InputManager.cpp
│       ├── InputManager.h
│       ├── StateManager.cpp
│       ├── StateManager.h
│       ├── WalletCore.cpp
│       └── WalletCore.h
└── tools/
    ├── wallet-terminal/
    │   └── wallet_terminal.py
    ├── clear-eeprom/
    │   └── clear-eeprom.ino
    ├── joystick-test/
    │   └── joystick-test.ino
    └── test-lcd/
        └── test-lcd.ino
```

This structure keeps the repository clean:

- `firmware/` contains the main ESP32 wallet firmware.
- `tools/` contains helper utilities and standalone test sketches.
- `docs/media/` contains photos and demo videos.
- `README.md` documents the project from the repository root.
- `LICENSE` contains the project license.

> Arduino IDE note: each `.ino` sketch should remain inside a folder with the same name as the sketch file. For example, `crypto-wallet-dual.ino` should remain inside a folder named `crypto-wallet-dual`.

## 🚀 Build and Upload

1. Install the ESP32 Arduino core.
2. Install required Arduino libraries.
3. Open the main firmware sketch:

```text
firmware/crypto-wallet-dual/crypto-wallet-dual.ino
```

4. Make sure all `.h` and `.cpp` files are in the same Arduino sketch folder.
5. Select the correct ESP32 board.
6. Select the correct serial port.
7. Compile and upload.

## ✅ Current Working Functionality

Currently implemented or partially implemented:

- TFT display initialization
- Main menu rendering
- Serial display mirroring
- Keyboard-arrow navigation through Python serial terminal
- PIN setup
- PIN unlock
- Wallet lock
- Prototype key generation
- Prototype address display
- Private key export for debugging
- PIN change flow
- Factory reset flow
- Info screen
- Wi-Fi credential entry
- Wi-Fi credential persistence
- Wi-Fi connectivity test
- EEPROM clear helper
- LCD test helper
- Joystick test helper
- Experimental Python serial terminal helper
- Partial joystick input integration

## 🛠️ Not Implemented Yet

Planned or incomplete:

- Full joystick-driven workflow
- Perfect serial terminal UI mirroring
- ATECC608A secure element integration
- Secure private key storage inside the secure element
- Secure signing flow
- Real blockchain transaction signing
- Standard blockchain address derivation
- Blockchain balance lookup
- Production-grade PIN retry limits
- Encrypted storage
- Secure backup/recovery phrase flow
- Tamper detection
- Battery/power management
- Final enclosure
- Full hardware schematic
- Complete threat model

## 🗺️ Roadmap

Planned development steps:

1. Add breadboard photo.
2. Add demo video.
3. Reorganize repository into `firmware/`, `tools/`, and `docs/media/`.
4. Improve Python terminal mirroring.
5. Finish joystick-driven navigation flow.
6. Add wiring diagram.
7. Clean and document helper sketches.
8. Integrate ATECC608A over I2C.
9. Test secure element detection.
10. Move private key generation/storage to ATECC608A.
11. Implement secure signing flow.
12. Replace prototype address generation with a standard derivation flow.
13. Add blockchain balance retrieval.
14. Add stricter PIN retry policy.
15. Add proper threat model documentation.
16. Design enclosure or compact hardware layout.

## 📝 Suggested Documentation Improvements

Recommended additional files:

```text
docs/
├── wiring.md
├── threat-model.md
├── roadmap.md
└── media/
    ├── breadboard-current.jpg
    └── current-demo.mp4
```

Recommended `wiring.md` contents:

- Component list
- Pin table
- Breadboard photo
- Power notes
- TFT wiring
- Joystick wiring
- ATECC608A planned wiring
- Known hardware issues

Recommended `threat-model.md` contents:

- What the prototype protects against
- What it does not protect against
- Current key storage limitations
- Physical attack limitations
- Firmware extraction risks
- Secure element integration plan

## ⚖️ Disclaimer

This project is an experimental hardware wallet prototype.

It is not production-ready.

Do not use it to store, manage, or sign transactions for real cryptocurrency funds.

The author is not responsible for loss of funds, misuse, hardware failure, insecure deployment, or incorrect assumptions about the security of this prototype.

## 📄 License

This project is licensed under the **PolyForm Noncommercial License 1.0.0**.

You may use, copy, modify, and share this software for personal, educational, research, and other non-commercial purposes.

Commercial use is not permitted without a separate commercial license.

For commercial licensing, contact the author through the GitHub profile associated with this repository.

Third-party dependencies are distributed under their respective licenses.

See the `LICENSE` file for details.
