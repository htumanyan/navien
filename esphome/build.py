#!/usr/bin/env python3
"""Interactive build script for Navien ESPHome firmware."""

import json
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent
BUILD_OUTPUT_DIR = PROJECT_ROOT / "build"
SECRETS_FILE = SCRIPT_DIR / "secrets.yaml"
PREFS_FILE = SCRIPT_DIR / ".build_prefs.json"


def find_esphome_command() -> list[str] | None:
    """Find how to run esphome. Returns command prefix or None."""
    # Check if esphome is directly available
    if shutil.which("esphome"):
        return ["esphome"]

    # Check if uv is available
    if shutil.which("uv"):
        return ["uv", "run", "esphome"]

    return None


def print_install_help() -> None:
    """Print installation instructions."""
    print("Error: Could not find esphome or uv")
    print()
    print("Install options:")
    print()
    print("  1) Install uv (recommended):")
    print("     curl -LsSf https://astral.sh/uv/install.sh | sh")
    print("     Then run: uv run python build.py")
    print()
    print("  2) Install esphome directly:")
    print("     pip install esphome")
    print()
    print("  3) Use Docker:")
    print("     docker run --rm -v \"${PWD}\":/config esphome/esphome compile navien.yml")

CONFIGS = {
    "1": ("navien.yml", "D1 Mini (ESP8266) - main config"),
    "2": ("navien-d1-mini.yml", "D1 Mini variant"),
    "3": ("navien-esphome-atom-lite-esp32.yml", "ESP32 Atom Lite"),
    "4": ("navien-esphome-atoms3-lite-tail485-esp32.yml", "ESP32 Atom S3 Lite with Tail485 module"),
    "5": ("navien-ht-device.yml", "Custom HT device"),
    "6": ("navien-wrd-hb.yml", "D1 Mini with hardwired hot button"),
}


def load_prefs() -> dict:
    """Load build preferences."""
    if PREFS_FILE.exists():
        try:
            return json.loads(PREFS_FILE.read_text())
        except (json.JSONDecodeError, IOError):
            pass
    return {}


def save_prefs(prefs: dict) -> None:
    """Save build preferences."""
    PREFS_FILE.write_text(json.dumps(prefs, indent=2) + "\n")


def prompt(message: str, default: str = "") -> str:
    """Prompt for input with optional default."""
    if default:
        result = input(f"{message} [{default}]: ").strip()
        return result if result else default
    return input(f"{message}: ").strip()


def prompt_secret(message: str) -> str:
    """Prompt for sensitive input."""
    import getpass
    return getpass.getpass(f"{message}: ")


def load_secrets() -> dict[str, str] | None:
    """Load existing secrets if available."""
    if not SECRETS_FILE.exists():
        return None

    secrets = {}
    with open(SECRETS_FILE) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and ":" in line:
                key, value = line.split(":", 1)
                # Remove quotes from value
                value = value.strip().strip('"').strip("'")
                secrets[key.strip()] = value
    return secrets


def save_secrets(secrets: dict[str, str]) -> None:
    """Save secrets to file."""
    with open(SECRETS_FILE, "w") as f:
        for key, value in secrets.items():
            f.write(f'{key}: "{value}"\n')
    print(f"Secrets saved to {SECRETS_FILE}")


def setup_secrets() -> bool:
    """Interactive secrets setup. Returns True if secrets are ready."""
    existing = load_secrets()

    if existing:
        print("\nExisting configuration found:")
        print(f"  WiFi SSID: {existing.get('wifi_ssid', '(not set)')}")
        print(f"  AP SSID:   {existing.get('ap_ssid', '(not set)')}")

        if prompt("\nUse existing configuration? (y/n)", "y").lower() == "y":
            return True
        print()

    print("WiFi Configuration")
    print("-" * 40)

    wifi_ssid = prompt("WiFi network name (SSID)")
    if not wifi_ssid:
        print("Error: WiFi SSID is required")
        return False

    wifi_password = prompt_secret("WiFi password")
    if not wifi_password:
        print("Error: WiFi password is required")
        return False

    print("\nFallback Access Point Configuration")
    print("-" * 40)
    print("(Used when the device can't connect to WiFi)")

    ap_ssid = prompt("Fallback AP name", "Navien-Fallback")
    ap_password = prompt_secret("Fallback AP password")
    if not ap_password or len(ap_password) < 8:
        print("Error: AP password must be at least 8 characters")
        return False

    secrets = {
        "wifi_ssid": wifi_ssid,
        "wifi_password": wifi_password,
        "ap_ssid": ap_ssid,
        "ap_password": ap_password,
    }

    save_secrets(secrets)
    return True


def select_config(prefs: dict) -> str | None:
    """Interactive config selection. Returns config filename or None."""
    # Find default from prefs
    default = "1"
    last_config = prefs.get("config")
    if last_config:
        for key, (filename, _) in CONFIGS.items():
            if filename == last_config:
                default = key
                break

    print("\nAvailable configurations:")
    print("-" * 40)
    for key, (filename, description) in CONFIGS.items():
        marker = "*" if key == default else " "
        print(f" {marker}{key}) {filename}")
        print(f"      {description}")
    print()

    choice = prompt(f"Select configuration (1-5)", default)

    if choice in CONFIGS:
        selected = CONFIGS[choice][0]
        prefs["config"] = selected
        save_prefs(prefs)
        return selected

    print(f"Invalid selection: {choice}")
    return None


def select_action() -> str | None:
    """Select build action."""
    print("\nActions:")
    print("-" * 40)
    print("  1) compile  - Build firmware only")
    print("  2) upload   - Upload to device (USB)")
    print("  3) run      - Build and upload")
    print("  4) logs     - View device logs")
    print()

    actions = {"1": "compile", "2": "upload", "3": "run", "4": "logs"}
    choice = prompt("Select action (1-4)", "1")

    return actions.get(choice)


def copy_firmware(config: str) -> None:
    """Copy firmware files to build output directory."""
    import shutil as sh

    # Parse device name from config
    device_name = None
    config_path = SCRIPT_DIR / config
    with open(config_path) as f:
        for line in f:
            if "device_name:" in line and not line.strip().startswith("#"):
                device_name = line.split(":", 1)[1].strip()
                break

    # Default device name from base config
    if not device_name:
        device_name = "navien-heater"

    # Find firmware files
    build_dir = SCRIPT_DIR / ".esphome" / "build" / device_name / ".pioenvs" / device_name
    if not build_dir.exists():
        print(f"Build directory not found: {build_dir}")
        return

    # Create output directory
    BUILD_OUTPUT_DIR.mkdir(exist_ok=True)

    # Copy firmware files
    copied = []
    for pattern, suffix in [("firmware.bin", ""), ("firmware.ota.bin", ".ota")]:
        src = build_dir / pattern
        if src.exists():
            dst = BUILD_OUTPUT_DIR / f"{device_name}{suffix}.bin"
            sh.copy2(src, dst)
            copied.append(dst)

    if copied:
        print("\nFirmware copied to:")
        for path in copied:
            print(f"  {path}")


def run_esphome(esphome_cmd: list[str], action: str, config: str) -> int:
    """Run esphome command."""
    cmd = esphome_cmd + [action, config]
    print(f"\nRunning: {' '.join(cmd)}")
    print("-" * 40)
    result = subprocess.call(cmd, cwd=SCRIPT_DIR)

    # Copy firmware after successful compile
    if result == 0 and action in ("compile", "run"):
        copy_firmware(config)

    return result


def main() -> int:
    print("Navien ESPHome Build Script")
    print("=" * 40)

    # Check for esphome availability
    esphome_cmd = find_esphome_command()
    if not esphome_cmd:
        print_install_help()
        return 1

    # Load preferences
    prefs = load_prefs()

    # Ensure secrets are configured
    if not setup_secrets():
        return 1

    # Select configuration
    config = select_config(prefs)
    if not config:
        return 1

    # Verify config exists
    config_path = SCRIPT_DIR / config
    if not config_path.exists():
        print(f"Error: Configuration file not found: {config_path}")
        return 1

    # Select action
    action = select_action()
    if not action:
        print("Invalid action")
        return 1

    # Run esphome
    return run_esphome(esphome_cmd, action, config)


if __name__ == "__main__":
    sys.exit(main())
