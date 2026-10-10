# CraftThingy Management & Automation Tools

This folder contains utilities for provisioning, testing, and managing CraftThingy ESP devices.

---

## 🚀 1. `esp_manager.html` (Recommended - Zero Installation)

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

## 🐍 2. `craftthingy_esp_ctl.py` (Optional Python CLI)

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

# Reboot device
python craftthingy_esp_ctl.py reboot --host 192.168.4.1 --key "<CURRENT_KEY>"
```

---

## 📦 3. `generate_release.py` (Release Packaging)

Prepares local PlatformIO build binaries for upload to GitHub Releases:
```bash
python generate_release.py --role water_pump --version 1.0.0
```
Renames binaries to the unified board tags (`firmware-water_pump-esp01s.bin`, `firmware-water_pump-esp32.bin`) and calculates companion `.sha256` checksum files.
