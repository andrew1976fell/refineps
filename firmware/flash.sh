#!/bin/bash
# flash.sh — Flash the current build output to the ESP32
#
# Flashes the three binaries produced by "idf.py build" from the build/
# directory. Uses esptool with --no-stub at 115200 baud — do not change
# these flags, idf.py flash does not work reliably on this machine.
#
# Usage:
#   bash firmware/flash.sh
#
# Prerequisites:
#   - Build must be up to date: bash -c "source ~/esp-idf-v5.5/export.sh && idf.py build"
#   - ESP32 connected via USB-UART adapter (default /dev/ttyUSB0)
#   - If port is wrong, check: ls /dev/ttyUSB* and set ESPPORT (see below)
#   - User in dialout group: sudo usermod -aG dialout $USER (then log out/in)
#
# Port/baud come from the standard esptool env vars ESPPORT/ESPBAUD, falling
# back to /dev/ttyUSB0 at 115200. Override without editing this file, e.g.:
#   ESPPORT=/dev/ttyUSB1 bash firmware/flash.sh
#
# The build directory is resolved relative to this script, so the repo can live
# at any path on any machine.
#
# To restore a known-good firmware instead of the current build, use restore.sh.
#
# Related:
#   firmware/CLAUDE.md              — authoritative flash reference and build rules
#   firmware/notes/setup.md         — first-time machine setup
#   firmware/restore.sh             — flash from backup instead of current build

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/build" && \
python3 -m esptool --chip esp32 --port "${ESPPORT:-/dev/ttyUSB0}" --baud "${ESPBAUD:-115200}" --no-stub \
  write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m \
  0x1000  bootloader/bootloader.bin \
  0x8000  partition_table/partition-table.bin \
  0x10000 refine_firmware.bin
