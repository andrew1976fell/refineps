# BLE Implementation Notes

## Overview

The app connects to the ESP32 firmware over BLE. The firmware device advertises as `RefinePS`.
All commands and responses are JSON strings. See [/notes/architecture.md](/notes/architecture.md) for the full command list.

## Permissions required (manifest)

- `BLUETOOTH_SCAN` — scanning for devices
- `BLUETOOTH_CONNECT` — connecting and communicating
- `ACCESS_FINE_LOCATION` — required by Android for BLE scanning (API < 31)

## Connection flow

1. Scan for a device named `RefinePS`
2. Connect to it via `BluetoothGatt`
3. Discover services
4. Find the notify characteristic → enable notifications
5. Find the write characteristic → use for sending commands
6. Send/receive JSON

## Fragmentation (important)

The ESP32 BLE MTU limits how much data fits in one packet. Large responses (e.g. telemetry) are split across multiple notify events by the firmware.

**Reassembly rule:** Buffer incoming notify data and only parse JSON once a complete `}` terminator is received. This was a known bug — fixed. Do not remove the reassembly logic.

## Known issues / watch points

- BLE permissions on Android 12+ changed — `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` are runtime permissions, not just manifest entries
- If the device doesn't appear in scan results, check that location permission is granted (Android bug, not a firmware issue)
- If connection drops silently, check `onConnectionStateChange` is wired up and retries or notifies the UI
