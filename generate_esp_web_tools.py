#!/usr/bin/env python3

import os
import json
import glob

def find_firmware_name(endswith=".bin"):
    firmware_directory = "web/firmware/"
    firmware_files = glob.glob(os.path.join(firmware_directory, f"*{endswith}"))
    if firmware_files:
        return os.path.splitext(os.path.basename(firmware_files[0]))[0]
    return None

def fill_manifest(github_username, github_project, github_build_number):
    firmware_name = find_firmware_name()
    if firmware_name is None:
        raise ValueError("No firmware files found in the 'web/firmware/' directory with '.bin' extension.")

    return {
        "name": f"{github_username}/{github_project}",
        "version": f"0.0.{github_build_number}",
        "funding_url": f"https://github.com/{github_username}/{github_project}/contributors",
        "builds": [
            {
                "chipFamily": "ESP32",
                "parts": [
                    {"path": f"firmware/{find_firmware_name('esp32.bin')}.bin", "offset": 0}
                ]
            },
            {
                "chipFamily": "ESP32-S3",
                "parts": [
                    {"path": f"firmware/{find_firmware_name('esp32s3.bin')}.bin", "offset": 0}
                ]
            }
        ]
    }

if __name__ == "__main__":
    # Retrieve values from GitHub Actions environment variables
    github_username, github_project = os.environ.get("GITHUB_REPOSITORY", 'user/repo').split('/')
    github_build_number = os.environ.get("GITHUB_RUN_NUMBER", '0')

    manifest_data = fill_manifest(github_username, github_project, github_build_number)

    output_file = "web/esp-webtools-manifest.json"

    with open(output_file, 'w') as file:
        json.dump(manifest_data, file, indent=2)

    print(f"Manifest has been filled and saved as {output_file}")
