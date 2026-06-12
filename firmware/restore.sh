#!/bin/bash
# restore.sh — Flash a known-good firmware from backup
#
# Flashes binaries from ~/Downloads/refineps_backup/ rather than the current
# build output. Use this to recover a working device when the current build
# is broken or untested.
#
# Usage:
#   bash firmware/restore.sh
#
# Prerequisites:
#   - Backup files must exist at $REFINE_BACKUP_DIR (default
#     $HOME/Downloads/refineps_backup/):
#       bootloader.bin, partition-table.bin, refine_firmware.bin
#   - ESP32 connected via USB-UART adapter (default /dev/ttyUSB0)
#   - If port is wrong, check: ls /dev/ttyUSB* and set ESPPORT (see below)
#
# Port/baud come from the standard esptool env vars ESPPORT/ESPBAUD, falling
# back to /dev/ttyUSB0 at 115200. The backup directory comes from
# REFINE_BACKUP_DIR. Override without editing this file, e.g.:
#   ESPPORT=/dev/ttyUSB1 REFINE_BACKUP_DIR=~/backups bash firmware/restore.sh
#
# To flash the current build output instead, use flash.sh.
#
# Related:
#   firmware/CLAUDE.md              — authoritative flash reference and build rules
#   firmware/flash.sh               — flash current build output

BACKUP_DIR="${REFINE_BACKUP_DIR:-$HOME/Downloads/refineps_backup}"
python3 -m esptool --chip esp32 --port "${ESPPORT:-/dev/ttyUSB0}" --baud "${ESPBAUD:-115200}" --no-stub \
  write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m \
  0x1000  "$BACKUP_DIR/bootloader.bin" \
  0x8000  "$BACKUP_DIR/partition-table.bin" \
  0x10000 "$BACKUP_DIR/refine_firmware.bin"
