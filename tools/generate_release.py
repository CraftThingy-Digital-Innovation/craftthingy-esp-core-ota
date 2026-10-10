#!/usr/bin/env python3
"""
CraftThingy Digital Innovation - Release Packaging & Asset Hashing Tool
Prepares firmware binaries with board tags and SHA-256 checksums for GitHub Releases.
"""

import argparse
import hashlib
import os
import shutil
import sys

BOARDS = [
    ("esp01_1m", "esp01s"),
    ("esp32dev",  "esp32"),
    ("esp32s3",   "esp32s3"),
    ("esp32c3",   "esp32c3"),
    ("esp32s2",   "esp32s2")
]

def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(8192):
            h.update(chunk)
    return h.hexdigest()

def package_release(build_dir: str, output_dir: str, role: str, version: str):
    os.makedirs(output_dir, exist_ok=True)
    manifest = {
        "version": version,
        "role": role,
        "assets": []
    }

    print(f"[*] Packaging release for role '{role}', version '{version}'...")

    for env_name, tag in BOARDS:
        src_bin = os.path.join(build_dir, env_name, "firmware.bin")
        if not os.path.exists(src_bin):
            print(f"[!] Warning: {src_bin} not found, skipping {env_name}")
            continue

        target_name = f"firmware-{role}-{tag}.bin" if role != "core" else f"firmware-{tag}.bin"
        dest_bin = os.path.join(output_dir, target_name)
        shutil.copy2(src_bin, dest_bin)

        file_hash = sha256_file(dest_bin)
        hash_file = dest_bin + ".sha256"
        with open(hash_file, "w") as f:
            f.write(f"{file_hash}  {target_name}\n")

        size = os.path.getsize(dest_bin)
        print(f"[+] Packaged {target_name} ({size} bytes, SHA256: {file_hash[:12]}...)")

        manifest["assets"].append({
            "board": env_name,
            "tag": tag,
            "filename": target_name,
            "size": size,
            "sha256": file_hash
        })

    print(f"[+] All release assets written to: {output_dir}")

def main():
    parser = argparse.ArgumentParser(description="Package CraftThingy release binaries")
    parser.add_argument("--build-dir", default=".pio/build", help="PlatformIO build directory")
    parser.add_argument("--out", default="releases", help="Output directory")
    parser.add_argument("--role", default="water_pump", help="Module role (e.g. core, water_pump, washing_machine)")
    parser.add_argument("--version", default="1.0.0", help="Release version tag")

    args = parser.parse_args()
    package_release(args.build_dir, args.out, args.role, args.version)

if __name__ == "__main__":
    main()
