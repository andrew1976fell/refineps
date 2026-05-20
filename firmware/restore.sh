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
#   - Backup files must exist at ~/Downloads/refineps_backup/:
#       bootloader.bin, partition-table.bin, refine_firmware.bin
#   - ESP32 connected via USB-UART adapter on /dev/ttyUSB0
#   - If port is wrong, check: ls /dev/ttyUSB* and edit --port below
#
# To flash the current build output instead, use flash.sh.
#
# Related:
#   firmware/CLAUDE.md              — authoritative flash reference and build rules
#   firmware/flash.sh               — flash current build output

python3 -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 115200 --no-stub \
  write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m \
  0x1000  /home/andrewandhelena/Downloads/refineps_backup/bootloader.bin \
  0x8000  /home/andrewandhelena/Downloads/refineps_backup/partition-table.bin \
  0x10000 /home/andrewandhelena/Downloads/refineps_backup/refine_firmware.bin
