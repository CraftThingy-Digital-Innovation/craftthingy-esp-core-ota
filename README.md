# CraftThingy Digital Innovation - Universal ESP Auto-Updater & Modular Controller Framework

[![Hardware](https://img.shields.io/badge/Hardware-ESP--01%20%7C%20ESP--01S%20%7C%20ESP32%20(All%20Variants)-blue.svg)]()
[![Security](https://img.shields.io/badge/Security-HMAC--SHA256%20Rolling%20Key%20Ratchet-red.svg)]()
[![OTA](https://img.shields.io/badge/OTA-GitHub%20Releases%20Streaming-green.svg)]()

High-efficiency, modular firmware and auto-updater framework for **ESP-01 / ESP-01S (1MB Flash)** and **ESP32 (all variants: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6)**.

---

## 🌟 Architecture & Key Features

1. **Clean Separation of Concerns**:
   - **Core Updater Engine (`craftthingy-esp-core-ota`)**: Manages WiFi provisioning, failover AP, cryptographic verification, rolling key ratchet, and GitHub OTA.
   - **Pluggable Application Controllers (`IDeviceController`)**: Dedicated hardware modules (e.g., Water Pump Controller, Washing Machine Controller, Relay Node) plug directly into the core framework without coupling.
   - **Two-Tier Update Engine**: The device can independently check and update its core firmware framework, and also pull the latest application updates for whatever role the controller is assigned to run.

2. **Ultra-Efficient Flash & RAM Budgeting**:
   - **Zero-HTML REST API**: Eliminates heavy web pages, saving tens of kilobytes of flash and RAM. Keeps firmware size well below 450 KB, fitting comfortably in the 512 KB 2-slot OTA partition of ESP-01/01S!
   - **Tuned BearSSL TLS**: Configured with 1024-byte streaming buffers on ESP8266, saving ~28 KB of heap memory during GitHub HTTPS connections.
   - **Streaming JSON & OTA Flashing**: Release metadata and firmware binaries stream directly from GitHub without ever being buffered entirely in RAM.

3. **Military-Grade API Authentication & Rolling Key Ratchet**:
   - **HMAC-SHA256 Request Signing**: Every sensitive API request signs `METHOD\nPATH\nTIMESTAMP\nNONCE\nBODY`.
   - **Rolling Key Forward Secrecy**: The device dynamically derives $K_{n+1} = \text{HMAC-SHA256}(K_n, \text{"CRAFT\_ROLL\_KEY\_"} + n)$ after configuration changes.
   - **Anti-Replay Filter**: Validates timestamps within a 5-minute window, enforces monotonic sequence counters, and checks a ring buffer of nonces.
   - **Multi-Tier Fallback**: Supports rolling HMAC keys, master recovery secret, and Bearer token headers.

4. **Reliable WiFi Failover & Captive AP**:
   - Starts in Station mode attempting connection to configured WiFi.
   - Retries up to 5 times with backoff.
   - If connection fails or no WiFi is configured: automatically launches SoftAP (`CraftThingy-XXXX` on `192.168.4.1`) with DNS server.
   - Periodically probes in the background to seamlessly reconnect if the router comes back online.

---

## 📁 Repository Structure

```
d:/CraftThingy/esp-stuff/
├── platformio.ini               # Multi-board build environments (ESP-01/01S, ESP32, S2, S3, C3)
├── include/
│   ├── CraftThingyConfig.h      # Hardware flags, board detection, default repo settings
│   └── Version.h                # Semantic version and build info
├── lib/
│   └── CraftThingyCore/         # Standalone, reusable C++ library (PlatformIO & Arduino IDE)
│       ├── library.json         # Library descriptor
│       └── src/
│           ├── CraftThingyCore.h / .cpp    # Master orchestrator
│           ├── ModuleDetector.h / .cpp     # Hardware fingerprinting & chip detection
│           ├── ConfigStore.h / .cpp        # EEPROM (ESP8266) & Preferences (ESP32)
│           ├── CraftThingyCrypto.h / .cpp  # Native HMAC-SHA256 & rolling key engine
│           ├── WiFiManager.h / .cpp        # Failover AP and retry state machine
│           ├── GitHubUpdater.h / .cpp      # 302-following GitHub OTA streaming flasher
│           ├── RestApi.h / .cpp            # Ultra-lean REST API server
│           ├── DeviceController.h          # Abstract interface for controllers
│           └── controllers/
│               ├── WaterPumpController.h      # Float sensor + relay + dry-run protection
│               └── WashingMachineController.h # Multi-stage cycle state machine
├── src/
│   └── main.cpp                 # Main firmware entry point
├── examples/
│   ├── WaterPumpController/WaterPumpController.ino
│   ├── WashingMachineController/WashingMachineController.ino
│   └── MinimalUpdaterOnly/MinimalUpdaterOnly.ino
└── tools/
    ├── craft_ctl.py             # Python CLI for signed requests and rolling key management
    └── generate_release.py      # Release packaging and SHA-256 asset generator
```

---

## 🛠️ Building & Flashing

### Using PlatformIO (Recommended)

```bash
# Build for ESP-01 / ESP-01S (1MB flash)
pio run -e esp01_1m

# Build for standard ESP32 (WROOM/DevKit)
pio run -e esp32dev

# Build for ESP32-S3
pio run -e esp32s3

# Build for ESP32-C3 (RISC-V)
pio run -e esp32c3

# Upload firmware
pio run -e esp01_1m -t upload
```

### Using Arduino IDE

1. Copy `lib/CraftThingyCore` into your Arduino `libraries/` directory.
2. Open any sketch from `examples/` (e.g. `examples/WaterPumpController/WaterPumpController.ino`).
3. Select your board:
   - For ESP-01/01S: Select **Generic ESP8266 Module**, Flash Size: **1MB (FS:64KB OTA:~470KB)**.
   - For ESP32: Select your specific ESP32 board and partition scheme **Minimal SPIFFS (1.9MB APP with OTA)**.
4. Click **Upload**.

---

## 📡 REST API Reference

All endpoints run on port 80 (e.g., `http://192.168.4.1` in AP mode, or the assigned local IP in Station mode).

| Method | Endpoint | Description | Auth Required |
|---|---|---|---|
| `GET` | `/api/status` | Hardware fingerprint, IP, RSSI, versions, OTA status | No |
| `GET` | `/api/wifi/scan` | Scans nearby WiFi networks and returns JSON array | No |
| `POST` | `/api/wifi` | Set WiFi SSID and password (attempts connection) | **Yes** |
| `POST` | `/api/github` | Change GitHub org, core repo, controller repo, or role | **Yes** |
| `POST` | `/api/update` | Trigger immediate GitHub release check and OTA flash | **Yes** |
| `POST` | `/api/key/rotate`| Explicitly advance rolling key ratchet to next sequence | **Yes** |
| `POST` | `/api/reboot` | Restart the device | **Yes** |

---

## 🔐 Using the Python Control Tool (`craft_ctl.py`)

The included Python tool handles HMAC-SHA256 request signing, timestamps, nonces, and rolling key tracking automatically.

### 1. Check Device Status
```bash
python tools/craft_ctl.py status --host 192.168.4.1
```

### 2. Scan Nearby WiFi Networks
```bash
python tools/craft_ctl.py scan-wifi --host 192.168.4.1
```

### 3. Configure WiFi Credentials
```bash
python tools/craft_ctl.py set-wifi --host 192.168.4.1 \
  --ssid "MyHomeWiFi" \
  --pass "StrongWifiPassword" \
  --key "a1b2c3d4e5f60718293a4b5c6d7e8f90112233445566778899aabbccddeeff00"
```

### 4. Change GitHub Target & Controller Role
```bash
python tools/craft_ctl.py set-github --host 192.168.4.1 \
  --org "CraftThingy-Digital-Innovation" \
  --core "craftthingy-esp-core-ota" \
  --ctrl "water-pump-controller" \
  --role "water_pump" \
  --key "a1b2c3d4e5f60718293a4b5c6d7e8f90112233445566778899aabbccddeeff00"
```
> **Note**: Whenever GitHub settings are modified, the rolling key automatically ratchets forward to sequence $n+1$ and the new key is outputted to the console!

### 5. Trigger Immediate OTA Update Check
```bash
python tools/craft_ctl.py trigger-update --host 192.168.4.1 --key "<CURRENT_KEY>"
```

---

## 📦 GitHub Release Asset Naming Convention

When publishing releases under `CraftThingy-Digital-Innovation`, upload compiled `.bin` files using the unified board tags:

| Target Hardware | Core Framework Binary | Controller Module Binary (e.g. Water Pump) |
|---|---|---|
| **ESP-01 / ESP-01S (1MB)** | `firmware-esp01s.bin` | `firmware-water_pump-esp01s.bin` |
| **Generic ESP8266** | `firmware-esp8266.bin` | `firmware-water_pump-esp8266.bin` |
| **Standard ESP32** | `firmware-esp32.bin` | `firmware-water_pump-esp32.bin` |
| **ESP32-S3** | `firmware-esp32s3.bin` | `firmware-water_pump-esp32s3.bin` |
| **ESP32-C3** | `firmware-esp32c3.bin` | `firmware-water_pump-esp32c3.bin` |
| **ESP32-S2** | `firmware-esp32s2.bin` | `firmware-water_pump-esp32s2.bin` |

Use `python tools/generate_release.py --role water_pump --version 1.0.1` to automatically package binaries and SHA-256 checksums ready for GitHub Releases.
