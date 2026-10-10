#!/usr/bin/env python3
"""
CraftThingy Digital Innovation - Device CLI & Rolling Key Security Tool
Provides HMAC-SHA256 request signing, rolling key ratcheting, WiFi configuration,
and GitHub OTA trigger for ESP8266 (ESP-01/01S) and ESP32.
"""

import argparse
import hashlib
import hmac
import json
import secrets
import sys
import time
import urllib.request
import urllib.error

DEFAULT_SECRET = "a1b2c3d4e5f60718293a4b5c6d7e8f90112233445566778899aabbccddeeff00"

def compute_hmac_signature(key: str, method: str, path: str, timestamp: str, nonce: str, body: str) -> str:
    payload = f"{method}\n{path}\n{timestamp}\n{nonce}\n{body}".encode("utf-8")
    key_bytes = key.encode("utf-8")
    return hmac.new(key_bytes, payload, hashlib.sha256).hexdigest()

def derive_next_rolling_key(current_key: str, sequence: int) -> str:
    msg = f"CRAFT_ROLL_KEY_{sequence}".encode("utf-8")
    return hmac.new(current_key.encode("utf-8"), msg, hashlib.sha256).hexdigest()

def send_signed_request(host: str, method: str, path: str, key: str, sequence: int, body_data: dict = None) -> dict:
    url = f"http://{host}{path}"
    body_str = json.dumps(body_data) if body_data else ""
    timestamp = str(int(time.time()))
    nonce = secrets.token_hex(8)

    sig = compute_hmac_signature(key, method, path, timestamp, nonce, body_str)

    headers = {
        "Content-Type": "application/json",
        "X-Signature": sig,
        "X-Timestamp": timestamp,
        "X-Nonce": nonce,
        "X-Sequence": str(sequence),
    }

    req = urllib.request.Request(
        url,
        data=body_str.encode("utf-8") if body_str else None,
        headers=headers,
        method=method
    )

    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            resp_body = resp.read().decode("utf-8")
            try:
                return json.loads(resp_body)
            except json.JSONDecodeError:
                return {"raw": resp_body}
    except urllib.error.HTTPError as e:
        err_msg = e.read().decode("utf-8")
        print(f"[-] HTTP Error {e.code}: {err_msg}", file=sys.stderr)
        sys.exit(1)
    except urllib.error.URLError as e:
        print(f"[-] Connection Error: {e.reason}", file=sys.stderr)
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="CraftThingy Device Management & Rolling Key Tool")
    parser.add_argument("--host", default="192.168.4.1", help="Device IP address (default: 192.168.4.1 in AP mode)")
    parser.add_argument("--key", default=DEFAULT_SECRET, help="Current Secret / Rolling Key")
    parser.add_argument("--seq", type=int, default=1, help="Rolling sequence counter (default: 1)")

    subparsers = parser.add_subparsers(dest="command", required=True)

    # Status
    subparsers.add_parser("status", help="Get device status, chip info, and versions")

    # WiFi Scan
    subparsers.add_parser("scan-wifi", help="Scan nearby WiFi networks")

    # Set WiFi
    wifi_parser = subparsers.add_parser("set-wifi", help="Set WiFi credentials")
    wifi_parser.add_argument("--ssid", required=True, help="WiFi SSID")
    wifi_parser.add_argument("--pass", dest="password", default="", help="WiFi Password")

    # Set GitHub
    gh_parser = subparsers.add_parser("set-github", help="Set GitHub repositories and active module role")
    gh_parser.add_argument("--org", default="CraftThingy-Digital-Innovation", help="GitHub Organization")
    gh_parser.add_argument("--core", default="craftthingy-esp-core-ota", help="Core Updater Repository")
    gh_parser.add_argument("--ctrl", default="water-pump-controller", help="Controller Application Repository")
    gh_parser.add_argument("--role", default="water_pump", help="Active role (e.g. water_pump, washing_machine)")
    gh_parser.add_argument("--token", default="", help="GitHub Personal Access Token (optional)")

    # Trigger Update
    subparsers.add_parser("trigger-update", help="Trigger immediate GitHub OTA update check")

    # Rotate Key
    subparsers.add_parser("rotate-key", help="Advance the rolling key ratchet to next sequence")

    # Set Master Bearer Token / Secret Key
    token_parser = subparsers.add_parser("set-token", help="Update the master Bearer token / secret key")
    token_parser.add_argument("--new-token", required=True, help="New secret Bearer token (min 16 chars)")

    master_parser = subparsers.add_parser("set-master-key", help="Update the device Master Secret Key")
    master_parser.add_argument("--new-master-key", required=True, help="New master secret key (min 16 chars, recommended 64-hex)")

    # Seed Rolling Key
    seed_parser = subparsers.add_parser("seed-key", help="Re-seed the rolling key ratchet with a new base seed & sequence")
    seed_parser.add_argument("--new-seed", required=True, help="New secret seed (min 16 chars, recommended 64-hex)")
    seed_parser.add_argument("--start-seq", type=int, default=1, help="Starting sequence counter (default: 1)")
    seed_parser.add_argument("--no-sync-master", dest="sync_master", action="store_false", help="Do NOT sync master secret key")
    seed_parser.set_defaults(sync_master=True)

    # Controller Commands
    subparsers.add_parser("pump-status", help="Get live water pump / controller telemetry")
    subparsers.add_parser("pump-stop", help="🚨 EMERGENCY INTERRUPT: Stop the pump immediately")
    subparsers.add_parser("pump-start", help="▶️ Manually force pump ON")
    subparsers.add_parser("pump-auto", help="🔄 RESUME AUTO float-switch mode")
    subparsers.add_parser("pump-reset-safety", help="🛡️ Reset dry-run safety alarm")

    # Set Device Name
    name_parser = subparsers.add_parser("set-name", help="Change device name and network hostname on the fly")
    name_parser.add_argument("--name", required=True, help="New device name / hostname (e.g. WaterPump-GroundFloor)")

    # Reboot
    subparsers.add_parser("reboot", help="Reboot device")

    args = parser.parse_args()

    if args.command == "status":
        res = send_signed_request(args.host, "GET", "/api/v1/system/status", args.key, args.seq)
        print("[+] Device Status:")
        print(json.dumps(res, indent=2))

    elif args.command == "scan-wifi":
        res = send_signed_request(args.host, "GET", "/api/v1/wifi/scan", args.key, args.seq)
        print("[+] Nearby WiFi Networks:")
        print(json.dumps(res, indent=2))

    elif args.command == "set-wifi":
        payload = {"ssid": args.ssid, "pass": args.password}
        res = send_signed_request(args.host, "POST", "/api/v1/wifi/config", args.key, args.seq, payload)
        print("[+] WiFi Configuration Result:")
        print(json.dumps(res, indent=2))

    elif args.command == "set-github":
        payload = {
            "org": args.org,
            "core_repo": args.core,
            "controller_repo": args.ctrl,
            "role": args.role,
            "token": args.token
        }
        res = send_signed_request(args.host, "POST", "/api/v1/github/config", args.key, args.seq, payload)
        print("[+] GitHub Configuration Result:")
        print(json.dumps(res, indent=2))
        data_block = res.get("data", {}) if isinstance(res.get("data"), dict) else res
        if "newRollingKey" in data_block:
            print(f"\n[!] IMPORTANT: Rolling Key ratcheted to sequence {data_block.get('newRollingSequence')}!")
            print(f"[!] SAVE YOUR NEW KEY: {data_block.get('newRollingKey')}")

    elif args.command == "trigger-update":
        res = send_signed_request(args.host, "POST", "/api/v1/ota/check", args.key, args.seq)
        print("[+] Update Trigger Result:")
        print(json.dumps(res, indent=2))

    elif args.command == "rotate-key":
        res = send_signed_request(args.host, "POST", "/api/v1/security/rotate-key", args.key, args.seq)
        print("[+] Key Rotation Result:")
        print(json.dumps(res, indent=2))
        data_block = res.get("data", {}) if isinstance(res.get("data"), dict) else res
        if "newRollingKey" in data_block:
            print(f"\n[!] New Key: {data_block.get('newRollingKey')}")

    elif args.command == "set-token":
        payload = {"token": args.new_token, "current_master_key": args.key}
        res = send_signed_request(args.host, "POST", "/api/v1/security/update-token", args.key, args.seq, payload)
        print("[+] Token Update Result:")
        print(json.dumps(res, indent=2))
        print(f"\n[!] Master Bearer token updated successfully! Use this token for external controllers.")

    elif args.command == "set-master-key":
        payload = {"token": args.new_master_key, "current_master_key": args.key}
        res = send_signed_request(args.host, "POST", "/api/v1/security/update-master-key", args.key, args.seq, payload)
        print("[+] Master Secret Key Update Result:")
        print(json.dumps(res, indent=2))
        print(f"\n[!] Master Secret Key successfully updated in flash storage!")
        print(f"[!] KEEP THIS SAFE: {args.new_master_key}")

    elif args.command == "set-name":
        payload = {"name": args.name}
        res = send_signed_request(args.host, "POST", "/api/v1/system/name", args.key, args.seq, payload)
        print("[+] Set Device Name Result:")
        print(json.dumps(res, indent=2))

    elif args.command == "seed-key":
        payload = {
            "seed": args.new_seed,
            "sequence": args.start_seq,
            "sync_master": args.sync_master
        }
        res = send_signed_request(args.host, "POST", "/api/v1/security/seed-key", args.key, args.seq, payload)
        print("[+] Rolling Key Re-Seed Result:")
        print(json.dumps(res, indent=2))
        print(f"\n[!] Rolling key re-seeded to sequence {args.start_seq}!")
        print(f"[!] SAVE YOUR NEW SEED/KEY: {args.new_seed}")

    elif args.command == "pump-status":
        res = send_signed_request(args.host, "GET", "/api/v1/device/status", args.key, args.seq)
        print("[+] Water Pump / Controller Telemetry:")
        print(json.dumps(res, indent=2))

    elif args.command == "pump-stop":
        res = send_signed_request(args.host, "POST", "/api/v1/device/command", args.key, args.seq, {"cmd": "interrupt"})
        print("[!] 🚨 EMERGENCY INTERRUPT RESULT:")
        print(json.dumps(res, indent=2))

    elif args.command == "pump-start":
        res = send_signed_request(args.host, "POST", "/api/v1/device/command", args.key, args.seq, {"cmd": "start"})
        print("[+] Pump Manual Start Result:")
        print(json.dumps(res, indent=2))

    elif args.command == "pump-auto":
        res = send_signed_request(args.host, "POST", "/api/v1/device/command", args.key, args.seq, {"cmd": "auto"})
        print("[+] Operation Resumed (Auto Mode):")
        print(json.dumps(res, indent=2))

    elif args.command == "pump-reset-safety":
        res = send_signed_request(args.host, "POST", "/api/v1/device/command", args.key, args.seq, {"cmd": "reset_safety"})
        print("[+] Safety Alarm Reset Result:")
        print(json.dumps(res, indent=2))

    elif args.command == "reboot":
        res = send_signed_request(args.host, "POST", "/api/v1/system/reboot", args.key, args.seq)
        print("[+] Reboot Result:")
        print(json.dumps(res, indent=2))

if __name__ == "__main__":
    main()
