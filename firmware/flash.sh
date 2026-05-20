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
#   - ESP32 connected via USB-UART adapter on /dev/ttyUSB0
#   - If port is wrong, check: ls /dev/ttyUSB* and edit --port below
#   - User in dialout group: sudo usermod -aG dialout $USER (then log out/in)
#
# To restore a known-good firmware instead of the current build, use restore.sh.
#
# Related:
#   firmware/CLAUDE.md              — authoritative flash reference and build rules
#   firmware/notes/setup.md         — first-time machine setup
#   firmware/restore.sh             — flash from backup instead of current build

cd /home/andrewandhelena/Downloads/refine-firmware/build && \
python3 -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 115200 --no-stub \
  write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m \
  0x1000  bootloader/bootloader.bin \
  0x8000  partition_table/partition-table.bin \
  0x10000 refine_firmware.bin
