# RefinePS Firmware — Lessons Learned & Best Practices

**Authoritative build, flash, and bug reference.** Do not duplicate this content elsewhere — update here first.  
Related: [firmware/refine_schema_v1.1.md](refine_schema_v1.1.md) | [firmware/notes/](notes/) | [notes/](../notes/)

## Contents

- [Hardware / Environment](#hardware--environment)
- [Working Flash Command](#working-flash-command)
- [Build Rules](#build-rules)
- [Known Bugs Fixed — Do Not Reintroduce](#known-bugs-fixed--do-not-reintroduce)
  - [1. esp_bt_controller_mem_release wrong mode](#1-esp_bt_controller_mem_release-wrong-mode-critical)
  - [2. PWM crash — LEDC resolution vs frequency](#2-pwm-crash--ledc-resolution-vs-frequency-critical)
  - [3. BLE JSON fragmentation](#3-ble-json-fragmentation)
  - [4. BLE connection drops](#4-ble-connection-drops-rsn-0x13)
- [Debugging Boot Loops](#debugging-boot-loops)
- [BLE Setup](#ble-setup)
- [Serial Monitor](#serial-monitor-read-only-no-idf.py-monitor)

---

## Hardware / Environment

- **ESP32** with 2MB flash, BLE only (Classic BT disabled)
- **Serial port** is `/dev/ttyUSB0` or `/dev/ttyUSB1` — check `ls /dev/ttyUSB*` before flashing
- `idf.py flash` and `idf.py monitor` do **not** work reliably on this machine
- `idf.py build` requires a fresh bash shell with ESP-IDF sourced:
  ```
  bash -c "source ~/esp-idf-v5.5/export.sh && idf.py build"
  ```

## Working Flash Command

Prefer the helper script — it resolves the build dir relative to itself, so it
works from any checkout path:
```
bash firmware/flash.sh
```

It always flashes with `--no-stub` at 115200 baud. Port and baud come from the
standard esptool env vars `ESPPORT`/`ESPBAUD` (default `/dev/ttyUSB0`, 115200);
override per-run without editing the script:
```
ESPPORT=/dev/ttyUSB1 bash firmware/flash.sh
```

Equivalent raw command (run from the firmware project root):
```
cd "$(git rev-parse --show-toplevel)/firmware/build" && \
python3 -m esptool --chip esp32 --port "${ESPPORT:-/dev/ttyUSB0}" --baud "${ESPBAUD:-115200}" --no-stub \
  write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m \
  0x1000  bootloader/bootloader.bin \
  0x8000  partition_table/partition-table.bin \
  0x10000 refine_firmware.bin
```

## Build Rules

1. **Always `idf.py fullclean` before rebuilding after any `sdkconfig` change.**
   Dirty rebuilds after sdkconfig changes leave stale objects with the old config compiled in.
   This caused a boot-loop regression that cost several sessions to diagnose.

2. When fullclean is needed but build environment isn't active, run from project root:
   ```
   bash -c "source ~/esp-idf-v5.5/export.sh && idf.py fullclean && idf.py build"
   ```

## Known Bugs Fixed — Do Not Reintroduce

### 1. `esp_bt_controller_mem_release` wrong mode (CRITICAL)
- **Symptom:** Immediate abort/boot-loop before app_main reaches any user code
- **Cause:** Calling `esp_bt_controller_mem_release(ESP_BT_MODE_BLE)` when Classic BT is
  disabled (`CONFIG_BT_CLASSIC_ENABLED=n`). IDF v5.4+ returns `ESP_ERR_NOT_SUPPORTED`
  which triggers `abort()` via `ESP_ERROR_CHECK`.
- **Fix:** The call is removed entirely. Classic BT is compiled out so there is nothing to release.
- **Rule:** Only call `esp_bt_controller_mem_release` for a mode that is actually compiled in.

### 2. PWM crash — LEDC resolution vs frequency (CRITICAL)
- **Symptom:** Crash at `pwm.c:96` on first call to `ledc_timer_config`
- **Cause:** 100 kHz carrier at 10-bit resolution (1023 steps) is not achievable on ESP32 —
  the hardware cannot divide the clock finely enough.
- **Fix:** Reduced to **8-bit resolution (255 steps) at 100 kHz**.
  All duty cycle calculations updated to use 0–255 range.
- **Rule:** On ESP32, 100 kHz PWM requires 8-bit resolution. Use the formula:
  `max_duty = (1 << resolution_bits) - 1`

### 3. BLE JSON fragmentation
- **Symptom:** Commands from nRF Connect arrive as separate 18-byte write events;
  each fragment was parsed individually and always failed.
- **Fix:** `bt_serial.c` now buffers incoming write fragments and only attempts JSON parse
  when the complete message has arrived, detected by the closing `}` character.
- **Rule:** BLE GATT write requests are MTU-limited (~20 bytes default). Never assume a
  single write event = a complete message.

### 4. BLE connection drops (rsn 0x13)
- **Symptom:** Connection dropped after a few seconds with reason code 0x13
  (remote user terminated connection).
- **Fix:** Connection parameter negotiation added; timeout increased.
- **Rule:** Always negotiate connection parameters after pairing; default iOS/Android
  supervision timeouts are aggressive.

## Debugging Boot Loops

When firmware crashes too fast to read serial output:
1. Add `vTaskDelay(pdMS_TO_TICKS(2000))` at the very start of `app_main()` — this gives
   the serial monitor time to connect before the crash.
2. Add `ESP_LOGI` before **every** init call so you can see exactly which one crashes.
3. Crashes stack — the first crash hides all later ones. Fix one at a time and rebuild.

## BLE Setup

- Device name: **RefinePS**
- Service UUID: `0xFF00`
- RX characteristic (phone → device): `0xFF01`
- TX characteristic (device → phone): `0xFF02`
- BLE only — Classic BT disabled in sdkconfig

## Serial Monitor (read-only, no idf.py monitor)

```
python3 -m serial.tools.miniterm /dev/ttyUSB0 115200
```
