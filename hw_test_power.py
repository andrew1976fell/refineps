#!/usr/bin/env python3
"""
RefinePS real-hardware power test — CH1, 12V battery, 135-ohm dummy load.
Steps through duty cycles and pauses at each for manual meter reading.
Ctrl-C at any pause sends alloff before exiting.
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
