#!/bin/bash
cd /home/andrewandhelena/Downloads/refine-firmware/build && python3 -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 115200 --no-stub write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m 0x1000 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin 0x10000 refine_firmware.bin
