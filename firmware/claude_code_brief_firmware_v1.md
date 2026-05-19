# Claude Code Brief — Refine PSU Firmware v1

## Task

Write ESP-IDF firmware for an ESP32 that implements the refine project
communications schema. This is the first working version — single channel
operational, three channels stubbed, dummy sensor values, Bluetooth Classic
serial transport.

---

## Environment

- Framework: ESP-IDF (not Arduino)
- Target: ESP32 (generic devkit)
- Transport: Bluetooth Classic SPP (Serial Port Profile)
- Build: standard idf.py build / flash / monitor

---

## Hardware

| Function | GPIO |
|---|---|
| Channel 1 PWM output | 25 |
| Channel 2 PWM output | 26 |
| Channel 3 PWM output | 27 |
| Status LED | 2 |

PWM peripheral: ESP32 LEDC
- Carrier frequency: 100000 Hz (100kHz)
- Switch frequency: 1000 Hz (1kHz) — alternates between duty_a and duty_b
- Resolution: 10 bit (0–1023 counts)
- Channels 2 and 3 stubbed — GPIO configured, PWM initialised, commands
  accepted and acknowledged, but duty held at 0 until future sessions

---

## Dual-Pulse PWM Architecture

Each channel has two duty cycle values: duty_a (high phase) and duty_b
(low phase). A 1kHz timer ISR alternates which duty cycle is active on
the LEDC output. At 100kHz carrier the cell's double-layer capacitance
integrates the pulses into two average voltage levels.

- duty_a = duty_b: equivalent to simple PWM
- freq_switch = 0: duty_a only, no switching
- All duty values are 0–100 percent, converted to LEDC counts internally

---

## Communications

Transport: Bluetooth Classic SPP
- Device name: "RefinePS"
- Accepts one JSON message per line (newline delimited)
- Sends one JSON response per line
- Sends telemetry once per second unprompted

All messages must contain "magic":"refine" — silently discard anything
without it. Do not send an error response for missing magic — just discard.

---

## Schema — Commands to implement

### set
```json
{"magic":"refine","cmd":"set","ch":1,"duty_a":80,"duty_b":20,
 "freq_carrier":100000,"freq_switch":1000}
```
- Apply duty_a, duty_b, freq_carrier, freq_switch to channel
- Missing fields use current channel values as defaults
- Respond with current channel state

### off
```json
{"magic":"refine","cmd":"off","ch":1}
```
- Set duty_a and duty_b to 0 on channel, stop switching
- Respond with current channel state

### alloff
```json
{"magic":"refine","cmd":"alloff"}
```
- Set all channels to 0, stop all switching
- No response body needed beyond acknowledgement

### status
```json
{"magic":"refine","cmd":"status"}
```
- Respond with full status of all three channels

### Unrecognised cmd
```json
{"magic":"refine","error":"bad command"}
```

---

## Schema — Responses to send

### Channel state (response to set / off)
```json
{"magic":"refine","ch":1,"duty_a":80,"duty_b":20,
 "freq_carrier":100000,"freq_switch":1000}
```

### Full status (response to status)
```json
{"magic":"refine","status":[
  {"ch":1,"duty_a":80,"duty_b":20,"freq_carrier":100000,"freq_switch":1000,
   "running":false,"profile":null},
  {"ch":2,"duty_a":0,"duty_b":0,"freq_carrier":100000,"freq_switch":1000,
   "running":false,"profile":null},
  {"ch":3,"duty_a":0,"duty_b":0,"freq_carrier":100000,"freq_switch":1000,
   "running":false,"profile":null}
]}
```

### Error
```json
{"magic":"refine","error":"bad command"}
{"magic":"refine","error":"bad channel"}
{"magic":"refine","error":"bad duty"}
{"magic":"refine","error":"bad freq"}
```

---

## Schema — Telemetry to send

Send once per second for each active channel (duty_a > 0 or duty_b > 0).
Sensor values are dummy constants for now — real ADC comes in a future session.

```json
{"magic":"refine","telem":true,"ch":1,"sensors":{"cell_v":0.0,"cell_a":0.0}}
```

---

## Error handling rules

- Missing magic field: discard silently, no response
- Bad channel (not 1/2/3): respond with bad channel error
- duty_a or duty_b outside 0–100: respond with bad duty error
- freq_carrier outside 1000–200000: respond with bad freq error
- freq_switch outside 0–10000: respond with bad freq error
- Unknown cmd string: respond with bad command error
- Malformed JSON (parse failure): discard silently, no response

---

## Status LED behaviour

| State | LED |
|---|---|
| BT not connected | slow blink 1Hz |
| BT connected, all channels off | steady on |
| BT connected, any channel active | fast blink 5Hz |

---

## Code structure

Organise into these components:
- `main.c` — entry point, init, task launch
- `bt_serial.c / .h` — Bluetooth SPP init and read/write
- `pwm.c / .h` — LEDC init, dual-pulse ISR, set/get channel state
- `schema.c / .h` — JSON parse, validate, dispatch, format responses
- `telemetry.c / .h` — 1 second timer, telemetry formatting and send

Use cJSON library (included in ESP-IDF components) for all JSON
parsing and generation.

---

## What to leave out

- Profile storage and execution — future session
- Preset lookup — future session
- Real ADC sensing — future session
- WiFi — future session
- OTA update — future session

The firmware must compile cleanly and run. Stub anything not yet
implemented with a comment marking it for the next session.

---

## Testing

After flash, verify with a Bluetooth serial terminal app on Android:
- Connect to "RefinePS"
- Send: `{"magic":"refine","cmd":"status"}`
- Expect: full status JSON response
- Send: `{"magic":"refine","cmd":"set","ch":1,"duty_a":50,"duty_b":25,"freq_carrier":100000,"freq_switch":1000}`
- Expect: channel 1 state response
- Observe: telemetry messages arriving every second
- Send: `{"magic":"refine","cmd":"alloff"}`
- Observe: telemetry stops

---

## Notes for Claude Code

- Use FreeRTOS tasks throughout — no blocking in main
- BT SPP receive in its own task, writes to a queue
- Schema dispatch reads from the queue
- Telemetry runs in its own timer task
- PWM ISR must be in IRAM — mark with IRAM_ATTR
- Use esp_log throughout with tag "REFINE"
- Target IDF v5.x API
