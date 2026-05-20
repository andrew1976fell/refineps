# Source Map

What each file does — start here when navigating the code.

## Entry point

| File | Role |
|------|------|
| `main/main.c` | `app_main()` — initialises BLE, PWM, telemetry in order |
| `main/main_ble.c` | BLE stack init, GATT server registration, connection callbacks |

## BLE layer

| File | Role |
|------|------|
| `main/bt_serial.c` | Receives BLE write events, reassembles fragments, passes complete JSON to schema layer |
| `main/bt_serial.h` | Public API for BLE serial receive and transmit |

Fragment reassembly lives here — do not remove it. See bug #3 in [CLAUDE.md](../CLAUDE.md).

## Command handling

| File | Role |
|------|------|
| `main/schema.c` | Parses incoming JSON commands, dispatches to PWM/telemetry, builds JSON responses |
| `main/schema.h` | Public API — `schema_handle_command(json_str)` |

The full command and response format is in [refine_schema_v1.1.md](../refine_schema_v1.1.md).

## PWM output

| File | Role |
|------|------|
| `main/pwm.c` | Configures ESP32 LEDC peripheral, sets duty cycles, handles dual-phase switching |
| `main/pwm.h` | Public API — `pwm_set_channel(ch, duty_a, duty_b, freq_carrier, freq_switch)` |

PWM is 8-bit resolution (0–255) at 100 kHz — do not change to 10-bit. See bug #2 in [CLAUDE.md](../CLAUDE.md).

## Telemetry

| File | Role |
|------|------|
| `main/telemetry.c` | Periodically reads sensors and sends JSON telemetry over BLE notify |
| `main/telemetry.h` | Public API — starts telemetry task |

`cell_v` and `cell_a` currently return dummy `0.0` — real ADC sensing not yet implemented.

## Build / config

| File | Role |
|------|------|
| `CMakeLists.txt` | Top-level CMake — sets project name |
| `main/CMakeLists.txt` | Lists all source files for the build |
| `main/idf_component.yml` | ESP-IDF component dependencies |
| `sdkconfig` | Generated build config — BLE only, Classic BT disabled |
| `sdkconfig.defaults` | Source-controlled defaults applied before `sdkconfig` is generated |

## Scripts

| File | Role |
|------|------|
| `flash.sh` | Convenience wrapper for the esptool flash command |
| `restore.sh` | Restores a known-good build state |
| `hw_test.py` | BLE test tool — connects to RefinePS, steps through all channels with 8-second pauses |
| `ble_cmd.py` | Send a single BLE command from the command line |
| `hw_test_power.py` | Power-focused hardware test variant |
