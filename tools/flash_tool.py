#!/usr/bin/env python3
"""
CraftThingy Digital Innovation - ESP Hardware Probing & Flashing Tool
=====================================================================
A unified cross-platform tool for probing, reading, backing up, erasing,
and flashing firmware onto ESP8266 (ESP-01 / ESP-01S) and ESP32 microcontrollers.

Requirements:
    pip install esptool pyserial
"""

import argparse
import os
import subprocess
import sys
import time

# Ensure UTF-8 output if supported, fallback safely on legacy Windows cp1252 consoles
if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass
if hasattr(sys.stderr, "reconfigure"):
    try:
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[-] Missing requirement 'pyserial'. Please install with: pip install pyserial", file=sys.stderr)
    sys.exit(1)


def find_esp_ports():
    """Scans and filters serial ports for common USB-UART bridges."""
    ports = serial.tools.list_ports.comports()
    matched = []
    keywords = ["ch340", "cp210", "ftdi", "usb-serial", "uart", "espressif", "ch341", "prolific"]
    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if any(k in desc or k in hwid for k in keywords):
            matched.append(p)
    return matched, ports


def check_and_assist_bootloader(port, baud=115200):
    """
    If the chip is actively running AT firmware or responsive over serial,
    attempts a software reset (AT+RST) so the user can enter bootloader mode
    simply by holding GPIO 0 to GND without unplugging the USB adapter.
    """
    try:
        s = serial.Serial(port, baud, timeout=0.2)
        s.write(b"AT\r\n")
        time.sleep(0.1)
        resp = s.read(s.in_waiting)
        if b"OK" in resp:
            print(f"[+] Detected responsive AT firmware on {port}!")
            print("[*] Triggering software reset (AT+RST) to check boot mode strapping...")
            s.write(b"AT+RST\r\n")
            time.sleep(0.4)
            boot_log = s.read(s.in_waiting).decode("latin1", "ignore")
            s.close()
            if "boot mode:(1," in boot_log:
                print("[+] SUCCESS: ESP8266 entered UART Bootloader Mode (boot mode: 1)!")
                return True
            elif "boot mode:(3," in boot_log:
                print("[!] Chip rebooted, but GPIO 0 is currently HIGH (boot mode: 3 - Normal Run).")
                print("    -> Hold Pin 3 (GPIO 0) grounded to Pin 1 (GND) to enter Download Mode (boot mode: 1).\n")
        else:
            s.close()
    except Exception:
        pass
    return False


def run_esptool(cmd_args):
    """Executes esptool using the current Python environment."""
    full_cmd = [sys.executable, "-m", "esptool"] + cmd_args
    print(f"[*] Executing: {' '.join(full_cmd)}\n")
    return subprocess.run(full_cmd)


def print_esp01_boot_instructions():
    """Prints step-by-step physical bootstrapping instructions for ESP-01."""
    print("=" * 70)
    print("[!] ESP-01 / ESP-01S BOOTSTRAPPING GUIDE (UART DOWNLOAD MODE)")
    print("=" * 70)
    print("Unlike ESP32 dev boards with auto-reset DTR/RTS circuitry, ESP-01")
    print("requires physical pin strapping to enter ROM bootloader mode:")
    print("")
    print("  Pinout (Antenna facing UP):")
    print("    Top Row:    [Pin 1: GND]   [Pin 2: GPIO 2]  [Pin 3: GPIO 0]  [Pin 4: RX]")
    print("    Bottom Row: [Pin 5: VCC]   [Pin 6: CH_PD]   [Pin 7: RST]     [Pin 8: TX]")
    print("")
    print("  HOW TO TRIGGER BOOTLOADER:")
    print("  Method A (Without unplugging):")
    print("    1. Connect a wire between Pin 3 (GPIO 0) and Pin 1 (GND).")
    print("    2. While holding that wire, re-run this tool: python tools/flash_tool.py probe")
    print("       (The tool will send a software reset; with GPIO 0 grounded, it boots to mode 1).")
    print("")
    print("  Method B (Power-cycle):")
    print("    1. UNPLUG the USB adapter.")
    print("    2. Bridge Pin 3 (GPIO 0) to Pin 1 (GND).")
    print("    3. PLUG the USB adapter back in WHILE holding GPIO 0 to GND.")
    print("    4. Re-run your probe or flash command.")
    print("=" * 70 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description="CraftThingy ESP-01 / ESP-01S / ESP32 Hardware Probe & Flashing Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Probe connected board details (auto-detects COM port):
  python flash_tool.py probe

  # Probe explicit COM port:
  python flash_tool.py --port COM18 probe

  # Flash core firmware binary to ESP-01 (1MB):
  python flash_tool.py flash --bin firmware-esp01s.bin

  # Erase entire flash (wipes corrupted storage or old EEPROM):
  python flash_tool.py erase

  # Backup 1MB flash contents to a binary file:
  python flash_tool.py backup --out esp01_backup.bin --size 1MB
"""
    )
    parser.add_argument("--port", "-p", default=None, help="Serial COM port (e.g. COM18, /dev/ttyUSB0). Auto-detected if omitted.")
    parser.add_argument("--baud", "-b", default="115200", help="Serial baud rate (default: 115200; use 460800 or 921600 for high-speed flashing)")
    parser.add_argument("--chip", "-c", default="esp8266", choices=["auto", "esp8266", "esp32", "esp32s2", "esp32s3", "esp32c3", "esp32c6"], help="Target SoC architecture (default: esp8266)")

    subparsers = parser.add_subparsers(dest="action", required=True)

    # 1. Probe
    subparsers.add_parser("probe", help="Probe connected SoC chip ID, MAC address, and flash capacity")

    # 2. Flash
    flash_parser = subparsers.add_parser("flash", help="Write a compiled .bin firmware image to flash")
    flash_parser.add_argument("--bin", required=True, help="Path to compiled firmware binary (.bin)")
    flash_parser.add_argument("--addr", default="0x0", help="Flash memory start address (default: 0x0 for ESP8266)")
    flash_parser.add_argument("--flash-mode", default="dout", choices=["dout", "dio", "qout", "qio"], help="SPI flash mode (default: dout - highest compatibility for ESP-01)")
    flash_parser.add_argument("--flash-size", default="1MB", help="SPI flash capacity (default: 1MB for ESP-01/ESP-01S, 4MB for ESP32)")
    flash_parser.add_argument("--verify", action="store_true", help="Verify flash contents against binary after writing")

    # 3. Erase
    subparsers.add_parser("erase", help="Erase entire SPI flash memory (clean wipe)")

    # 4. Backup
    backup_parser = subparsers.add_parser("backup", help="Dump SPI flash memory to a local binary file")
    backup_parser.add_argument("--out", default="flash_dump.bin", help="Output binary file path (default: flash_dump.bin)")
    backup_parser.add_argument("--addr", default="0x0", help="Start address (default: 0x0)")
    backup_parser.add_argument("--size", default="1MB", help="Number of bytes/size to read (e.g. 1MB, 4MB)")

    args = parser.parse_args()

    # Determine Port
    port = args.port
    if not port:
        matched_ports, all_ports = find_esp_ports()
        if matched_ports:
            port = matched_ports[0].device
            print(f"[+] Auto-detected programmer port: {port} ({matched_ports[0].description})")
        elif all_ports:
            port = all_ports[0].device
            print(f"[!] No explicit ESP device match. Using available port: {port} ({all_ports[0].description})")
        else:
            print("[-] No serial COM ports detected on this computer!", file=sys.stderr)
            sys.exit(1)

    # Software-assisted bootloader check (if chip is running AT firmware)
    check_and_assist_bootloader(port, int(args.baud))

    # ACTION: PROBE
    if args.action == "probe":
        print(f"\n[*] Probing hardware on {port} (Baud: {args.baud}, Chip: {args.chip})...")
        chip_args = ["--chip", args.chip] if args.chip != "auto" else []
        
        # 1. Flash ID & Manufacturer
        res = run_esptool(["--port", port, "--baud", args.baud] + chip_args + ["flash-id"])
        if res.returncode != 0:
            print("\n[-] Probe failed: Communication sync timed out or was rejected.")
            print_esp01_boot_instructions()
            sys.exit(1)

        # 2. Read MAC
        print("\n[*] Reading device MAC address...")
        run_esptool(["--port", port, "--baud", args.baud] + chip_args + ["read-mac"])

        # 3. Chip ID (if esp8266)
        if args.chip in ["esp8266", "auto"]:
            print("\n[*] Reading ESP8266 Chip ID...")
            run_esptool(["--port", port, "--baud", args.baud] + chip_args + ["chip-id"])

        print("\n[+] Hardware probe completed successfully!")

    # ACTION: FLASH
    elif args.action == "flash":
        if not os.path.isfile(args.bin):
            print(f"[-] Firmware binary not found: '{args.bin}'", file=sys.stderr)
            sys.exit(1)

        bin_size = os.path.getsize(args.bin)
        print(f"\n[*] Preparing to flash '{args.bin}' ({bin_size:,} bytes) to {port} at {args.addr}...")
        
        # 2-Slot OTA size guard check for 1MB ESP-01
        if args.flash_size == "1MB" and bin_size > 491520:  # ~480KB limit
            print(f"[!] WARNING: Firmware size ({bin_size:,} bytes) exceeds standard ESP-01 2-slot OTA limit (~480 KB)!")
            print("    OTA self-updates will fail if partition boundary is crossed.")

        chip_args = ["--chip", args.chip] if args.chip != "auto" else []
        cmd = [
            "--port", port,
            "--baud", args.baud,
        ] + chip_args + [
            "write-flash",
            "--flash-mode", args.flash_mode,
            "--flash-size", args.flash_size,
            args.addr,
            args.bin
        ]

        res = run_esptool(cmd)
        if res.returncode == 0:
            print("\n[+] Flashing completed successfully!")
            print("[!] Disconnect GPIO 0 from GND and reboot to run application.")
        else:
            print("\n[-] Flashing failed!")
            print_esp01_boot_instructions()
            sys.exit(1)

    # ACTION: ERASE
    elif args.action == "erase":
        print(f"\n[*] Erasing flash on {port}...")
        chip_args = ["--chip", args.chip] if args.chip != "auto" else []
        res = run_esptool(["--port", port, "--baud", args.baud] + chip_args + ["erase-flash"])
        if res.returncode == 0:
            print("\n[+] Flash memory erased completely!")
        else:
            print("\n[-] Flash erase failed!")
            print_esp01_boot_instructions()
            sys.exit(1)

    # ACTION: BACKUP
    elif args.action == "backup":
        print(f"\n[*] Backing up flash memory from {port} to '{args.out}'...")
        chip_args = ["--chip", args.chip] if args.chip != "auto" else []
        res = run_esptool([
            "--port", port,
            "--baud", args.baud
        ] + chip_args + [
            "read-flash",
            args.addr,
            args.size,
            args.out
        ])
        if res.returncode == 0:
            print(f"\n[+] Backup saved successfully to '{args.out}' ({os.path.getsize(args.out):,} bytes).")
        else:
            print("\n[-] Backup failed!")
            print_esp01_boot_instructions()
            sys.exit(1)


if __name__ == "__main__":
    main()
