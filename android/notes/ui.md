# UI Notes

## Stack

Jetpack Compose with Material 3. State managed via `ViewModel` (`lifecycle-viewmodel-compose`).

## Screens / structure (planned)

```
App
├── ScanScreen       — scan for RefinePS device, show connect button
├── ControlScreen    — main screen once connected
│   ├── ChannelCard  — one per channel: duty slider + on/off toggle
│   ├── TelemetryBar — shows cell_v and cell_a readings
│   └── AllOff button
└── ProfileScreen    — (not yet built) upload/run/stop profiles
```

## Current state

- BLE connect is working
- UI for channel control: in progress
- Telemetry display: not yet built
- Profile screen: not yet started

## State model (intended)

- `BleViewModel` owns BLE connection and exposes `StateFlow` to the UI
- UI is stateless — reads from ViewModel, sends events back via callbacks
- Each channel has: `duty: Int`, `isOn: Boolean`

## Things to keep consistent with firmware

- Channel numbering starts at 0 or 1 — check firmware `{"cmd":"set","ch":N}` — pick one and stick to it
- `alloff` response is `{"magic":"refine","ok":true}` — don't expect the command echoed back
- Telemetry `cell_v` and `cell_a` are currently `0.0` in firmware — UI should handle zero gracefully without showing it as an error
