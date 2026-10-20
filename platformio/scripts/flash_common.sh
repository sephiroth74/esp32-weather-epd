# Shared by backup_flash.sh and restore_flash.sh: esptool from PlatformIO,
# download mode instructions, serial port detection and board identification.

BACKUP_DIR="${BACKUP_DIR:-$HOME/esp32-backups}"

DOWNLOAD_MODE_HELP='Download mode
  The board must be in download mode: the ROM bootloader runs instead of the
  firmware, so the board stays on USB (with native USB the port disappears
  while the firmware is in deep sleep).

  1. Connect the board to the computer with a USB data cable.
  2. Hold the BOOT button.
  3. Press and release the RESET button, while still holding BOOT.
  4. Release BOOT.
  Or: hold BOOT while you plug in the USB cable, then release it.

  Seeed XIAO ESP32-C6     the two tiny buttons next to the USB-C connector:
                          "B" is BOOT, "R" is RESET.
  Waveshare ESP32-C6-Zero BOOT and RESET buttons on the top side.
  Waveshare ESP32-S3-Zero BOOT and RESET buttons on the top side.

  The board then shows up in `pio device list` (/dev/cu.usbmodem* on macOS)
  and the display doesn'"'"'t refresh. It stays in download mode until RESET is
  pressed (or the restore resets it).'

download_mode_hint() {
  echo >&2
  echo "$DOWNLOAD_MODE_HELP" >&2
}

# esptool from PlatformIO's package. Every call keeps the board in the
# bootloader afterwards (--after no-reset) unless the caller overrides it:
# after a reset the firmware would run and could go to deep sleep before the
# next command.
# NO_COLOR: with TERM=xterm-256color esptool writes ANSI cursor and color codes
# even when its output is captured; on a real terminal it still uses them.
esptool() {
  NO_COLOR=1 pio pkg exec -p tool-esptoolpy -- esptool --after no-reset "$@"
}

# Prints the serial port of the only ESP32 attached over USB (Espressif native
# USB or a common USB-UART bridge), or fails listing the candidates.
detect_port() {
  local ports count
  ports=$(pio device list --json-output | python3 -I -c '
import json, re, sys
for device in json.load(sys.stdin):
    if re.search(r"VID:PID=(303A|10C4|1A86|0403):", device.get("hwid", ""), re.I):
        print(device["port"])')
  count=$(printf '%s' "$ports" | grep -c .)
  if [ "$count" -eq 1 ]; then
    echo "$ports"
    return 0
  fi
  if [ "$count" -eq 0 ]; then
    echo "No ESP32 found on USB." >&2
    download_mode_hint
  else
    echo "More than one ESP32 found, choose one with -p:" >&2
    echo "$ports" >&2
  fi
  return 1
}

# Reads the board on port $1 into CHIP (e.g. esp32c6), MAC (base MAC, hex
# digits only), FLASH_BYTES and BOARD_INFO (esptool's full output).
read_board_info() {
  local port=$1 out mb
  if ! out=$(esptool --port "$port" flash-id 2>&1); then
    echo "$out" >&2
    echo "Could not talk to the board on $port." >&2
    download_mode_hint
    return 1
  fi
  # drop any ANSI escape codes left, so that the lines start with their label
  out=$(printf '%s\n' "$out" | sed $'s/\x1b\\[[0-9;]*[A-Za-z]//g')
  BOARD_INFO=$out
  CHIP=$(printf '%s\n' "$out" | sed -nE 's/^(Connected to|Chip is) ([^ :]+).*/\2/p' | head -1 | tr 'A-Z' 'a-z' | tr -d '-')
  # chips with an EUI-64 (ESP32-C6) print it as MAC, then the base MAC
  MAC=$(printf '%s\n' "$out" | sed -nE 's/^BASE MAC: *([0-9a-f:]+).*/\1/p' | head -1)
  [ -n "$MAC" ] || MAC=$(printf '%s\n' "$out" | sed -nE 's/^MAC: *([0-9a-f:]+).*/\1/p' | head -1)
  MAC=$(printf '%s' "$MAC" | tr -d ':')
  mb=$(printf '%s\n' "$out" | sed -nE 's/^Detected flash size: ([0-9]+)MB.*/\1/p' | head -1)
  if [ -z "$CHIP" ] || [ -z "$MAC" ] || [ -z "$mb" ]; then
    echo "$out" >&2
    echo "Could not read the chip, MAC or flash size from esptool's output." >&2
    return 1
  fi
  FLASH_BYTES=$((mb * 1024 * 1024))
}
