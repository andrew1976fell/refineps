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
- Service UUID and characteristic UUID defined in firmware — check `main/ble.c` for values
- Commands are JSON strings sent as BLE writes; responses are JSON notify events
- Large responses are fragmented by firmware and reassembled by Android app

## Key commands

| Command | Effect |
|---------|--------|
| `{"cmd":"status"}` | Returns channel states and telemetry |
| `{"cmd":"set","ch":N,"duty":D}` | Set channel N to duty cycle D |
| `{"cmd":"off","ch":N}` | Turn off channel N |
| `{"cmd":"alloff"}` | Turn off all channels |
| `{"cmd":"telemetry"}` | Returns cell_v and cell_a readings |

## Hardware

- MCU: ESP32
- PWM channels: GPIO 25, 26, 27 (verify with meter — not yet confirmed in hardware)
- ADC sensing: not yet wired to real hardware (dummy 0.0 values in firmware)
