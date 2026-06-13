# Session handoff — 2026-06-13

Working record from a cloud-container Claude Code session, written so the next
session (local CC on the Lubuntu bench machine) can pick up cleanly. Delete or
overwrite this once acted on — it is a transient handoff, not a permanent note.

## Where things stand

- Branch: **`firmware/path-independent-flash-scripts`**, pushed and tracking
  `origin`. Working tree clean. Latest commit `7612c2f`.
- A repo-root **`CLAUDE.md`** was added this session as the project-wide
  orientation map (defers to `firmware/CLAUDE.md` for firmware detail).
- Both original outstanding tasks are now **done**:
  - Build verified path-independent — flash/restore scripts resolve dirs via
    `$BASH_SOURCE` (commit `416e3f9`), `build/` is untracked so its absolute
    CMake paths never enter the repo, and the only env reference is the
    home-relative `~/esp-idf-v5.5/`.
  - Branch committed and pushed.

## Firmware state (unchanged this session)

- PWM carrier default **1 kHz** (pre-prototype bench test, commit `4f351ce`).
- BLE fragment reassembly in place (`bt_serial.c`, newline-delimited).
- Boot loop fixed (removed bad `esp_bt_controller_mem_release(ESP_BT_MODE_BLE)`).

## Not verifiable from the cloud container

- **ESP32 USB connection could not be checked here.** This session ran in a dev
  container (`/workspaces/refineps`, user `codespace`) with no access to the host
  USB bus — no `/dev/ttyUSB*`, no `/dev/ttyACM*`, `lsusb` not installed. This is
  expected, not a fault.

## Pick up here on local CC (Lubuntu)

1. Confirm the board is connected:
   ```
   ls -l /dev/ttyUSB* ; lsusb | grep -iE 'cp210|ch340|ftdi|silicon|qinheng'
   ```
   Expect a `/dev/ttyUSB0` (or `ttyUSB1`) and a USB-UART bridge. If the device
   node exists but flashing hits a permission error, fix the dialout group:
   `sudo usermod -aG dialout $USER` then log out/in.
2. Build, then flash:
   ```
   bash -c "source ~/esp-idf-v5.5/export.sh && idf.py build"   # from firmware/
   bash firmware/flash.sh                                       # from repo root
   ```
   Use `ESPPORT=/dev/ttyUSB1 bash firmware/flash.sh` if the port differs.
3. Read serial (no `idf.py monitor` on this machine):
   ```
   python3 -m serial.tools.miniterm /dev/ttyUSB0 115200
   ```
4. Open a PR for the branch if ready:
   `https://github.com/andrew1976fell/refineps/pull/new/firmware/path-independent-flash-scripts`
