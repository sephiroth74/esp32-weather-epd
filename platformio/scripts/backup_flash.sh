#!/usr/bin/env zsh
# Backs up the whole flash of a board (bootloader, partition table, firmware,
# NVS) so that restore_flash.sh can put it back exactly.

set -u
start_dir=$PWD
cd "$(dirname "$0")/.."
. scripts/flash_common.sh

usage() {
  cat <<EOF
Usage: scripts/backup_flash.sh [-p PORT] [-o DIR] [NAME]

  NAME     labels the backup, e.g. the env name (default: board)
  -p PORT  serial port, e.g. /dev/cu.usbmodem1101 (default: the only ESP32
           found on USB)
  -o DIR   directory for the backup (default: \$BACKUP_DIR, or
           ~/esp32-backups if that isn't set); created if missing

The backup is saved as DIR/NAME_CHIP_MAC_DATE.bin, with the board info in a
.txt next to it, and is verified against the flash. It contains the WiFi password and the API key:
keep it out of the repository.

Afterwards the board stays in download mode: upload the new firmware now
(pio run -e ENV -t upload), or press RESET to run the current one.

$DOWNLOAD_MODE_HELP
EOF
}

port=""
while getopts "p:o:h" opt; do
  case $opt in
    p) port=$OPTARG ;;
    o) BACKUP_DIR=$OPTARG ;;
    h) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done
shift $((OPTIND - 1))
name=${1:-board}

if [ -z "$port" ]; then
  port=$(detect_port) || exit 1
fi
read_board_info "$port" || exit 1

# relative to where the script was started, not to the project
case $BACKUP_DIR in
  /*) ;;
  *) BACKUP_DIR=$start_dir/$BACKUP_DIR ;;
esac
mkdir -p "$BACKUP_DIR" && chmod 700 "$BACKUP_DIR" || exit 1
# the backup holds credentials: not in a git working tree, unless ignored there
if git -C "$BACKUP_DIR" rev-parse --is-inside-work-tree > /dev/null 2>&1 \
    && ! git -C "$BACKUP_DIR" check-ignore -q .; then
  echo "$BACKUP_DIR is inside a git repository and not ignored there:" >&2
  echo "the backup contains the WiFi password and the API key. Choose another -o DIR." >&2
  rmdir "$BACKUP_DIR" 2> /dev/null
  exit 1
fi
file="$BACKUP_DIR/${name}_${CHIP}_${MAC}_$(date +%Y%m%d-%H%M%S).bin"

echo "Backing up $((FLASH_BYTES / 1024 / 1024)) MB of flash of $CHIP ($MAC) on $port"
esptool --port "$port" read-flash 0 "$FLASH_BYTES" "$file" || exit 1
chmod 600 "$file"
printf '%s\n' "$BOARD_INFO" > "${file%.bin}.txt"
chmod 600 "${file%.bin}.txt"

echo "Verifying the backup against the flash"
if ! esptool --port "$port" verify-flash 0 "$file"; then
  echo "Verification FAILED: don't rely on $file" >&2
  exit 1
fi

echo
echo "Backup OK: $file"
echo "The board is still in download mode: upload the new firmware now, or press RESET."
