#!/usr/bin/env python3
"""
hw_test_power.py — RefinePS real-hardware power test

Steps through duty cycles on CH1 with a real load attached, pausing at
each step for a manual meter reading. Interactive — you press Enter to
advance. Ctrl-C at any pause safely sends alloff before exiting.

Use this AFTER hw_test.py confirms BLE and PWM are working without a load.

Usage:
  pip install bleak                  # one-time install
  python3 hw_test_power.py

Hardware setup:
  - 12V supply or battery on the ESP32 power rail
  - 135-ohm dummy load resistor across CH1 output (GPIO 25)
  - DMM in DC-V mode across the load
  - ESP32 flashed and advertising BLE as RefinePS

Test sequence (CH1 only, GPIO 25):
  alloff first — confirm 0V before starting
  10%  steady  →  ~1.2V  /  ~9mA
  25%  steady  →  ~3.0V  / ~22mA
  50%  steady  →  ~6.0V  / ~44mA
  75%  steady  →  ~9.0V  / ~67mA
  100% steady  → ~12.0V  / ~89mA  (1.1W in resistor)
  Dual-pulse 80/20 @ 1kHz — scope needed to see switching
  50%  again   →  ~6.0V  — repeatability check
  alloff on exit

All readings are for a 12V supply with 135-ohm load.
Adjust expected voltages proportionally for different supply voltages.

Related:
  firmware/hw_test.py            — BLE-only test, no load required, run this first
  firmware/notes/hardware.md     — GPIO map and hardware verification checklist
  firmware/CLAUDE.md             — BLE UUIDs and build/flash reference
  firmware/refine_schema_v1.1.md — full command/response protocol
"""
import asyncio, json
from bleak import BleakScanner, BleakClient

DEVICE_NAME = "RefinePS"
RX_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"
TX_UUID = "0000ff02-0000-1000-8000-00805f9b34fb"

LOAD_OHMS = 135
SUPPLY_V  = 12.0

# (duty_a, duty_b, freq_switch, description)
STEPS = [
    (10,  10,  0,    "10%  steady  →  ~1.2 V  /  ~9 mA"),
    (25,  25,  0,    "25%  steady  →  ~3.0 V  / ~22 mA"),
    (50,  50,  0,    "50%  steady  →  ~6.0 V  / ~44 mA"),
    (75,  75,  0,    "75%  steady  →  ~9.0 V  / ~67 mA"),
    (100, 100, 0,    "100% steady  → ~12.0 V  / ~89 mA   (1.1 W in resistor)"),
    (80,  20,  1000, "Dual-pulse 80/20 @ 1 kHz switch — avg ~9.6 V, scope shows alternating duty"),
    (50,  50,  0,    "50%  again   →  ~6.0 V  — confirm matches earlier reading"),
]

class _RxBuffer:
    def __init__(self):
        self._buf = ""

    def __call__(self, sender, data):
        self._buf += data.decode(errors="replace")
        dec = json.JSONDecoder()
        while True:
            s = self._buf.lstrip()
            if not s:
                self._buf = ""
                break
            try:
                msg, end = dec.raw_decode(s)
                print(f"  << {json.dumps(msg)}")
                self._buf = s[end:]
            except json.JSONDecodeError:
                self._buf = s
                break

rx = _RxBuffer()

async def send(client, cmd):
    raw = (json.dumps(cmd) + "\n").encode()
    print(f"  >> {json.dumps(cmd)}")
    await client.write_gatt_char(RX_UUID, raw)
    await asyncio.sleep(0.5)

async def run():
    print("Scanning for RefinePS...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
    if not device:
        print("ERROR: RefinePS not found — is it powered and advertising?")
        return

    print(f"Found: {device.address}")
    async with BleakClient(device) as client:
        await client.start_notify(TX_UUID, rx)
        await asyncio.sleep(0.5)

        print("\nSending alloff — confirm no voltage across load before continuing.")
        await send(client, {"magic": "refine", "cmd": "alloff"})

        try:
            input("\n  DMM in DC-V mode across load.  Enter to start, Ctrl-C to abort: ")

            for duty_a, duty_b, freq_switch, label in STEPS:
                print(f"\n{'─' * 62}")
                print(f"  {label}")
                await send(client, {
                    "magic": "refine", "cmd": "set", "ch": 1,
                    "duty_a": duty_a, "duty_b": duty_b,
                    "freq_carrier": 100000, "freq_switch": freq_switch,
                })
                input("  Read meter — Enter to advance, Ctrl-C to alloff+exit: ")

            print(f"\n{'─' * 62}")
            print("All steps complete.")

        except KeyboardInterrupt:
            print("\nAborted by user.")

        finally:
            print("Sending alloff...")
            await send(client, {"magic": "refine", "cmd": "alloff"})
            await client.stop_notify(TX_UUID)
            print("Output off. Safe to disconnect.")

asyncio.run(run())
