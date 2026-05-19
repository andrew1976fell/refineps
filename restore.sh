#!/bin/bash
python3 -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 115200 --no-stub \
  write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m \
  0x1000  /home/andrewandhelena/Downloads/refineps_backup/bootloader.bin \
  0x8000  /home/andrewandhelena/Downloads/refineps_backup/partition-table.bin \
  0x10000 /home/andrewandhelena/Downloads/refineps_backup/refine_firmware.bin
