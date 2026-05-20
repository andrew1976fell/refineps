#!/usr/bin/env python3
"""
hw_test.py — RefinePS hardware verification test

Steps through a full PWM test sequence on all three channels (GPIO 25, 26, 27),
pausing 8 seconds at each step so you can read a meter or scope on the GPIO pin.

Usage:
  pip install bleak          # one-time install
  python3 hw_test.py

Requirements:
  - RefinePS firmware flashed and advertising over BLE
  - bleak installed (pip install bleak)
  - Multimeter or oscilloscope connected to GPIO 25/26/27

Test sequence:
  CH1 (GPIO 25): status → 50% → 25% → 75% → dual-pulse 80/20 → off
  CH2 (GPIO 26): 50% steady
  CH3 (GPIO 27): 50% steady
  alloff → final status

Expected readings (12V supply example):
  duty 50%  → ~6.0V average
  duty 25%  → ~3.0V average
  duty 75%  → ~9.0V average
  dual 80/20 → alternates ~9.6V / ~2.4V at 1kHz (scope needed to see switching)

Related:
  firmware/notes/hardware.md     — GPIO map and hardware verification checklist
  firmware/CLAUDE.md             — BLE UUIDs and build/flash reference
  firmware/refine_schema_v1.1.md — full command/response protocol
"""
import asyncio, json, time
from bleak import BleakScanner, BleakClient

DEVICE_NAME = "RefinePS"
SVC_UUID    = "0000ff00-0000-1000-8000-00805f9b34fb"
RX_UUID     = "0000ff01-0000-1000-8000-00805f9b34fb"
TX_UUID     = "0000ff02-0000-1000-8000-00805f9b34fb"

PAUSE = 8.0

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

async def send(client, cmd: dict):
    raw = (json.dumps(cmd) + "\n").encode()
    print(f"\n>> {json.dumps(cmd)}")
    await client.write_gatt_char(RX_UUID, raw)
    await asyncio.sleep(PAUSE)

async def main():
    print("Scanning for RefinePS...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
    if not device:
        print("ERROR: RefinePS not found — is it advertising?")
        return

    print(f"Found: {device.address}")
    async with BleakClient(device) as client:
        print("Connected. Subscribing to TX notifications...")
        await client.start_notify(TX_UUID, rx)
        await asyncio.sleep(1.0)

        print("\n=== HARDWARE TEST SEQUENCE ===")
        print("Use a meter/scope on GPIO 25 (ch1), 26 (ch2), 27 (ch3)")

        await send(client, {"magic": "refine", "cmd": "status"})

        print("\n--- CH1: 50% steady (duty_a=duty_b=50, no switching) ---")
        print("Probe GPIO 25 — expect ~50% PWM")
        await send(client, {"magic": "refine", "cmd": "set", "ch": 1,
                            "duty_a": 50, "duty_b": 50,
                            "freq_carrier": 100000, "freq_switch": 0})

        print("\n--- CH1: 25% steady ---")
        print("Probe GPIO 25 — expect ~25% PWM")
        await send(client, {"magic": "refine", "cmd": "set", "ch": 1,
                            "duty_a": 25, "duty_b": 25,
                            "freq_carrier": 100000, "freq_switch": 0})

        print("\n--- CH1: 75% steady ---")
        print("Probe GPIO 25 — expect ~75% PWM")
        await send(client, {"magic": "refine", "cmd": "set", "ch": 1,
                            "duty_a": 75, "duty_b": 75,
                            "freq_carrier": 100000, "freq_switch": 0})

        print("\n--- CH1: dual-pulse 80/20 at 1 kHz switch ---")
        print("Probe GPIO 25 — expect alternating 80% / 20% at 1 kHz")
        await send(client, {"magic": "refine", "cmd": "set", "ch": 1,
                            "duty_a": 80, "duty_b": 20,
                            "freq_carrier": 100000, "freq_switch": 1000})

        print("\n--- CH1 off ---")
        await send(client, {"magic": "refine", "cmd": "off", "ch": 1})

        print("\n--- CH2: 50% steady ---")
        print("Probe GPIO 26 — expect ~50% PWM")
        await send(client, {"magic": "refine", "cmd": "set", "ch": 2,
                            "duty_a": 50, "duty_b": 50,
                            "freq_carrier": 100000, "freq_switch": 0})

        print("\n--- CH3: 50% steady ---")
        print("Probe GPIO 27 — expect ~50% PWM")
        await send(client, {"magic": "refine", "cmd": "set", "ch": 3,
                            "duty_a": 50, "duty_b": 50,
                            "freq_carrier": 100000, "freq_switch": 0})

        print("\n--- All off ---")
        await send(client, {"magic": "refine", "cmd": "alloff"})

        print("\n--- Final status ---")
        await send(client, {"magic": "refine", "cmd": "status"})

        print("\n=== TEST COMPLETE ===")
        await client.stop_notify(TX_UUID)

asyncio.run(main())
