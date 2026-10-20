#!/usr/bin/env zsh
# Restores a backup made with backup_flash.sh, overwriting the whole flash.

set -u
start_dir=$PWD
cd "$(dirname "$0")/.."
. scripts/flash_common.sh

usage() {
  cat <<EOF
Usage: scripts/restore_flash.sh [-p PORT] [-y] [-f] FILE

  FILE     a backup made by backup_flash.sh (NAME_CHIP_MAC_DATE.bin)
  -p PORT  serial port, e.g. /dev/cu.usbmodem1101 (default: the only ESP32
           found on USB)
  -y       don't ask for confirmation
  -f       restore even if FILE comes from another board (chip or MAC
           differ) or doesn't match the size of its flash

The whole flash is overwritten: firmware, partition table and NVS (location
index, WiFi calibration). The board is then reset and runs the restored
firmware.

$DOWNLOAD_MODE_HELP
EOF
}

port=""
yes=0
force=0
while getopts "p:yfh" opt; do
  case $opt in
    p) port=$OPTARG ;;
    y) yes=1 ;;
    f) force=1 ;;
    h) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done
shift $((OPTIND - 1))
if [ $# -ne 1 ]; then
  usage >&2
  exit 2
fi
file=$1
# relative to where the script was started, not to the project
case $file in
  /*) ;;
  *) file=$start_dir/$file ;;
esac
if [ ! -f "$file" ]; then
  echo "No such file: $file" >&2
  exit 1
fi

if [ -z "$port" ]; then
  port=$(detect_port) || exit 1
fi
read_board_info "$port" || exit 1

mismatch=0
size=$(wc -c < "$file" | tr -d ' ')
if [ "$size" -ne "$FLASH_BYTES" ]; then
  echo "$file is $size bytes, the flash of this board is $FLASH_BYTES bytes." >&2
  mismatch=1
fi
# NAME may contain underscores: CHIP and MAC are the 3rd and 2nd last fields
base=$(basename "$file" .bin)
file_chip=$(echo "$base" | awk -F_ '{print $(NF-2)}')
file_mac=$(echo "$base" | awk -F_ '{print $(NF-1)}')
if [ "$file_chip" != "$CHIP" ] || [ "$file_mac" != "$MAC" ]; then
  echo "$file was taken from $file_chip $file_mac, this board is $CHIP $MAC." >&2
  mismatch=1
fi
if [ "$mismatch" -eq 1 ] && [ "$force" -ne 1 ]; then
  echo "Not restoring. Use -f to restore anyway." >&2
  exit 1
fi

if [ "$yes" -ne 1 ]; then
  printf 'Overwrite the whole flash of %s (%s) on %s with %s? [y/N] ' \
    "$CHIP" "$MAC" "$port" "$file"
  read -r answer
  case $answer in
    y|Y|yes) ;;
    *) echo "Aborted."; exit 1 ;;
  esac
fi

# keep the image header flash settings as they are in the backup; the last
# --after wins over esptool()'s no-reset, so the restored firmware boots
esptool --port "$port" --after hard-reset write-flash \
  --flash-mode keep --flash-freq keep --flash-size keep 0 "$file" || exit 1

echo
echo "Restored $file. The board has been reset and runs the restored firmware."
