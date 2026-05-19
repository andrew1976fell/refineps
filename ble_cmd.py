#!/usr/bin/env python3
"""
Send a single BLE command to RefinePS and print the response.
Usage:
  python3 ble_cmd.py alloff
  python3 ble_cmd.py set 50          # CH1 steady 50%
  python3 ble_cmd.py set 80 20       # CH1 dual-pulse 80/20 @ 1kHz
  python3 ble_cmd.py status
"""
import asyncio, json, sys
from bleak import BleakScanner, BleakClient

DEVICE_NAME = "RefinePS"
RX_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"
TX_UUID = "0000ff02-0000-1000-8000-00805f9b34fb"

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
                print(f"<< {json.dumps(msg)}")
                self._buf = s[end:]
            except json.JSONDecodeError:
                self._buf = s
                break

rx = _RxBuffer()

async def run(args):
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
    if not device:
        print("ERROR: RefinePS not found")
        return

    async with BleakClient(device) as client:
        await client.start_notify(TX_UUID, rx)
        await asyncio.sleep(0.3)

        if args[0] == "alloff":
            cmd = {"magic": "refine", "cmd": "alloff"}
        elif args[0] == "status":
            cmd = {"magic": "refine", "cmd": "status"}
        elif args[0] == "set":
            duty_a = int(args[1])
            duty_b = int(args[2]) if len(args) > 2 else duty_a
            freq_switch = 1000 if duty_a != duty_b else 0
            cmd = {"magic": "refine", "cmd": "set", "ch": 1,
                   "duty_a": duty_a, "duty_b": duty_b,
                   "freq_carrier": 100000, "freq_switch": freq_switch}
        else:
            print(f"Unknown command: {args[0]}")
            return

        print(f">> {json.dumps(cmd)}")
        await client.write_gatt_char(RX_UUID, (json.dumps(cmd) + "\n").encode())
        await asyncio.sleep(1.5)
        await client.stop_notify(TX_UUID)

asyncio.run(run(sys.argv[1:]))
