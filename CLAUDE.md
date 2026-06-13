# RefinePS

Read this first every session. It is the project-wide orientation map.
For anything firmware-specific (flash commands, build rules, known bugs), the
authoritative source is **[firmware/CLAUDE.md](firmware/CLAUDE.md)** — do not
contradict it. This file summarises; that file is the detail.

## What this project is

RefinePS is a bench power-supply / signal control project with two components:

- **firmware/** — ESP32 firmware (ESP-IDF 5.5, BLE only, Classic BT disabled).
  Drives a PWM carrier and reports telemetry over BLE. Device name `RefinePS`,
  service `0xFF00`, RX `0xFF01`, TX `0xFF02`.
- **android/** — Kotlin/Compose Android app (`com.andrewscrap.refineps`) that
  connects over BLE to send commands and display telemetry.

Cross-project notes live in **[notes/](notes/)** (repo-map, roadmap,
architecture, decisions). Rule across the repo: *one truth* — write a fact in
one place, then make code/config match it.

## Current firmware state

- **PWM carrier: 1 kHz** — default lowered from 100 kHz for the pre-prototype
  bench test (commit `4f351ce`). Note: `firmware/CLAUDE.md` still describes the
  100 kHz / 8-bit constraint as historical context; the *active* default is 1 kHz.
- **BLE fragmentation fix in place** — `bt_serial.c` reassembles GATT write
  fragments (newline-delimited) before parsing JSON; single writes are no longer
  assumed to be complete messages.
- **Boot loop fixed** — the bad `esp_bt_controller_mem_release(ESP_BT_MODE_BLE)`
  call (aborted when Classic BT is compiled out) was removed. Firmware now boots
  cleanly. See "Known Bugs Fixed" in firmware/CLAUDE.md before touching BLE init.

Hardware pins (pre-prototype rig): GPIO25 PWM, GPIO33 voltage ADC, GPIO32
current ADC.

## Outstanding tasks

- **Make the build path-independent** — the flash/restore scripts and
  firmware/CLAUDE.md are already resolved relative to their own location
  (commit `416e3f9`); confirm the build itself has no hard-coded checkout paths.
- **Commit and push** — current branch `firmware/path-independent-flash-scripts`
  has no upstream and is not yet pushed. Commit this CLAUDE.md and push the branch.

## Flash command (Lubuntu)

Build, then flash with the helper (resolves the build dir relative to itself):

```bash
bash -c "source ~/esp-idf-v5.5/export.sh && idf.py build"   # from firmware/
bash firmware/flash.sh                                       # from repo root
```

Override the serial port per run without editing the script (check
`ls /dev/ttyUSB*` first):

```bash
ESPPORT=/dev/ttyUSB1 bash firmware/flash.sh
```

`idf.py flash` / `idf.py monitor` are unreliable on this machine — use
`flash.sh` (esptool, `--no-stub`, 115200) and `python3 -m serial.tools.miniterm
/dev/ttyUSB0 115200` for the monitor. Full detail in firmware/CLAUDE.md.

## ESP-IDF location (Lubuntu)

ESP-IDF v5.5 is installed at **`~/esp-idf-v5.5/`**. Source it in a fresh shell
before any `idf.py` command:

```bash
source ~/esp-idf-v5.5/export.sh
```
