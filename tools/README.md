# CraftThingy Management, Flashing & Automation Tools

This folder contains the complete operational tooling suite for probing hardware, flashing firmware binaries, provisioning network credentials, and managing CraftThingy ESP devices.

---

## ⚡ 1. `flash_tool.py` (Hardware Probe & Binary Flasher)

A cross-platform Python utility built on `esptool` and `pyserial` for probing silicon hardware, erasing SPI flash, backing up memory, and flashing compiled binaries directly onto ESP8266 (ESP-01 / ESP-01S) and ESP32 devices over a USB-to-UART bridge.

### Prerequisites:
```bash
pip install esptool pyserial
```

### 🚨 Crucial ESP-01 / ESP-01S Bootstrapping Pinout:
Standard development boards (like NodeMCU or ESP32 DevKit) include automatic DTR/RTS auto-reset transistors. **ESP-01 / ESP-01S modules do NOT have auto-reset circuits.**
To put an ESP-01 into UART Download (flashing) mode:
1. **Unplug** the USB-to-UART adapter (e.g. CH340 / CP2102).
2. **Bridge GPIO 0 to GND**:
   - If your adapter has a toggle switch: Slide it to **`PROG`** (not `UART`).
   - If your adapter has pin headers or you are wiring directly: Connect **GPIO 0 (Pin 5) to GND (Pin 1)**.
   - Verify **CH_PD / EN (Pin 4)** is tied to **3.3V (VCC)**.
3. **Plug the adapter back into your USB port** while GPIO 0 is grounded. The chip boots into ROM bootloader mode (`boot mode:(1,x)`).
4. Run your probe or flash command.
5. **After flashing completes**: Unplug, remove the GPIO 0 jumper (or toggle back to `UART`), and plug back in to run the firmware.

---

### Commands & Usage:

#### 🔍 A. Probe Hardware (Chip ID, MAC, Flash Memory Capacity)
Auto-detects the connected COM port and queries silicon specs:
```bash
# Auto-detects COM port:
python flash_tool.py probe

# Or specify COM port explicitly:
python flash_tool.py --port COM17 probe

# For ESP32 targets:
python flash_tool.py --chip esp32 probe
```
*Output reveals:*
- Target SoC model (e.g. `ESP8266EX`)
- Device MAC Address (e.g. `84:f3:eb:xx:xx:xx`)
- SPI Flash Manufacturer & Device ID (e.g. Winbond / Puya `0x1440e0`)
- Detected flash capacity (e.g. `1MB / 8Mbit`)

#### ⚡ B. Flash Firmware Binary
Writes the compiled firmware `.bin` to flash with automatic safety checks:
```bash
# Flash ESP-01 / ESP-01S (1MB, default start address 0x0):
python flash_tool.py flash --bin firmware-esp01s.bin

# High-speed flash with verification (460800 baud):
python flash_tool.py --baud 460800 flash --bin firmware-esp01s.bin --verify

# Flash standard ESP32:
python flash_tool.py --chip esp32 flash --bin firmware-esp32.bin --flash-size 4MB
```
> [!NOTE]
> For ESP-01 / ESP-01S, `--flash-mode dout` is selected by default to ensure 100% compatibility across all SPI flash memory chip revisions (PUYA, GigaDevice, Winbond).

#### 🧹 C. Erase Flash (Clean Wipe)
Completely wipes all sectors of the SPI flash. Recommended if the device was previously loaded with incompatible code or corrupt EEPROM settings:
```bash
python flash_tool.py erase
```

#### 💾 D. Backup Flash Memory
Dumps the current physical flash contents to a binary file before testing experimental builds:
```bash
python flash_tool.py backup --out backup_esp01.bin --size 1MB
```

#### 🛠️ Troubleshooting Connection Timeouts (`Connecting...................`):
If `esptool` fails to sync and outputs `Failed to connect to Espressif device: Serial data stream stopped`:
1. Check device manager for the COM port (e.g. `COM17` using CH340).
2. Unplug the adapter.
3. Verify GPIO 0 is physically grounded to GND.
4. Plug back in and retry immediately.
5. Ensure baud rate is `115200`.

---

## 🚀 2. `esp_manager.html` (Zero Installation Browser Dashboard)

A self-contained, single-file browser dashboard for configuring ESP devices. **Does not require Python or any software installation.**

### How to Use:
1. Connect your computer or phone to the ESP's WiFi hotspot:
   - SSID: `CraftThingy-XXXX`
   - Default IP: `192.168.4.1`
2. Double-click [`esp_manager.html`](./esp_manager.html) in Windows Explorer to open it in Chrome, Edge, or Firefox.
3. Features:
   - **Live Telemetry**: Displays detected chip, flash size, free RAM, IP, and firmware versions.
   - **One-Click WiFi Scanner**: Finds nearby networks so you can select your SSID and enter the password.
   - **GitHub Reconfiguration**: Change target organizations, core repos, and application repos.
   - **In-Browser WebCrypto**: Calculates HMAC-SHA256 signatures, timestamps, and nonces natively inside your browser.
   - **Rolling Key Ratchet**: Advance the rolling key with one click and copy the new key to your clipboard.
   - **Check OTA Now / Reboot**: Trigger immediate firmware checks or restart the microcontroller.

---

## 🐍 3. `craftthingy_esp_ctl.py` (REST API & Automation CLI)

A Python command-line utility for developers who want to script or automate API calls from a terminal.

### Requirements:
Python 3.8+ (uses only built-in standard libraries: `hmac`, `hashlib`, `urllib`, `secrets`).

### Commands:
```bash
# Check device status (system, network, OTA, attached controller)
python craftthingy_esp_ctl.py status --host 192.168.4.1

# Scan nearby WiFi networks
python craftthingy_esp_ctl.py scan-wifi --host 192.168.4.1

# Configure WiFi credentials with HMAC signature
python craftthingy_esp_ctl.py set-wifi --host 192.168.4.1 \
  --ssid "HomeNetwork" \
  --pass "Password123" \
  --key "a1b2c3d4e5f60718293a4b5c6d7e8f90112233445566778899aabbccddeeff00"

# Water Pump / Hardware Controller Controls
python craftthingy_esp_ctl.py pump-status --host 192.168.4.1
python craftthingy_esp_ctl.py pump-stop --host 192.168.4.1           # 🚨 Emergency Stop / Interrupt
python craftthingy_esp_ctl.py pump-start --host 192.168.4.1          # ▶️ Force Pump ON
python craftthingy_esp_ctl.py pump-auto --host 192.168.4.1           # 🔄 Resume Float Switch Auto Mode
python craftthingy_esp_ctl.py pump-reset-safety --host 192.168.4.1   # 🛡️ Reset Dry-Run Safety Alarm

# Change GitHub target & role (automatically advances rolling key)
python craftthingy_esp_ctl.py set-github --host 192.168.4.1 \
  --org "CraftThingy-Digital-Innovation" \
  --core "craftthingy-esp-core-ota" \
  --ctrl "craftthingy-esp-water-pump" \
  --role "water_pump" \
  --key "<CURRENT_KEY>"

# Trigger an immediate OTA update check
python craftthingy_esp_ctl.py trigger-update --host 192.168.4.1 --key "<CURRENT_KEY>"

# Manually advance rolling key ratchet
python craftthingy_esp_ctl.py rotate-key --host 192.168.4.1 --key "<CURRENT_KEY>"

# Update Master Bearer Token (for Home Assistant / External Controller)
python craftthingy_esp_ctl.py set-token --host 192.168.4.1 --new-token "MySecretKey2026!" --key "<CURRENT_KEY>"

# Update Master Secret Key directly (Dual-key protection enforces current master key)
python craftthingy_esp_ctl.py set-master-key --host 192.168.4.1 \
  --new-master-key "f9e8d7c6b5a403211234567890abcdef1234567890abcdef1234567890abcdef" \
  --key "<CURRENT_KEY>"

# Re-seed the Rolling Key Ratchet (Brand new seed & sequence counter)
python craftthingy_esp_ctl.py seed-key --host 192.168.4.1 \
  --new-seed "e1f2a3b4c5d60718293a4b5c6d7e8f90112233445566778899aabbccddeeff11" \
  --start-seq 1 \
  --key "<CURRENT_KEY>"

# Reboot device
python craftthingy_esp_ctl.py reboot --host 192.168.4.1 --key "<CURRENT_KEY>"
```

---

## 📦 4. `generate_release.py` (Release Packaging)

Prepares local PlatformIO build binaries for upload to GitHub Releases:
```bash
python generate_release.py --role water_pump --version 1.0.0
```
Renames binaries to the unified board tags (`firmware-water_pump-esp01s.bin`, `firmware-water_pump-esp32.bin`) and calculates companion `.sha256` checksum files.
