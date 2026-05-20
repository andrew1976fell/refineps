# Architecture

## System overview

```
[ Power supply hardware ]
        |
        | GPIO / ADC
        v
[ ESP32 firmware ]  ←— BLE —→  [ Android app ]
  (firmware/)                     (android/)
```

## BLE protocol

- Device name: `RefinePS`
- Service UUID: `0xFF00`
- RX characteristic (app → ESP32): `0xFF01`
- TX characteristic (ESP32 → app): `0xFF02`
- Every message in both directions must include `"magic":"refine"` — firmware silently discards anything without it
- Commands are JSON strings sent as BLE writes; responses are JSON notify events
- Large responses are fragmented by firmware and reassembled by Android app

For the full command and response reference see [firmware/refine_schema_v1.1.md](/firmware/refine_schema_v1.1.md).

## Key commands (summary only)

| Command | Effect |
|---------|--------|
| `{"magic":"refine","cmd":"status"}` | Returns all channel states |
| `{"magic":"refine","cmd":"set","ch":1,"duty_a":80,"duty_b":20,"freq_carrier":100000,"freq_switch":1000}` | Set channel |
| `{"magic":"refine","cmd":"off","ch":1}` | Turn off one channel |
| `{"magic":"refine","cmd":"alloff"}` | Turn off all channels |

## Hardware

- MCU: ESP32 (2MB flash, BLE only — Classic BT disabled)
- PWM channels: GPIO 25 (ch1), GPIO 26 (ch2), GPIO 27 (ch3) — not yet verified with meter
- ADC sensing (cell_v, cell_a): not yet wired — firmware returns dummy 0.0
