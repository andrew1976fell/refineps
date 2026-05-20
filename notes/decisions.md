# Decisions

Key technical choices and why — so future-you doesn't re-debate settled questions.

## Single GitHub repo for firmware + Android

**Decision:** One repo (`refineps`) with `firmware/` and `android/` subfolders.  
**Why:** Single `git pull` keeps both sides in sync across machines. No cross-repo dependency headaches.  
**Revisit if:** The Android app or firmware grows large enough that independent release cadences become painful.

## BLE fragmentation handled in Android app

**Decision:** Firmware sends large responses in fragments; Android reassembles before parsing JSON.  
**Why:** ESP32 BLE MTU limits packet size. Firmware is simpler if it just sends; Android handles reassembly.  
**Revisit if:** Multiple clients need to talk to the firmware — then push reassembly into firmware.

## esptool with --no-stub at 115200 (not idf.py flash)

**Decision:** Flash with `esptool.py --no-stub -b 115200` on the Lubuntu machine.  
**Why:** `idf.py flash` does not work reliably on this machine (driver/stub issue). See firmware/CLAUDE.md.  
**Revisit if:** Moving to a different machine or USB-UART adapter.

## JSON over BLE (not a binary protocol)

**Decision:** All commands and responses are JSON strings.  
**Why:** Human-readable, easy to debug with a BLE scanner, easy to extend without version negotiation.  
**Revisit if:** Bandwidth or latency becomes a problem (unlikely for a bench power supply).
