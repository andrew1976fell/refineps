# Refine Project — Communications Schema v1.1

**Authoritative BLE protocol reference.** Do not duplicate this content elsewhere — update here first.  
Related: [firmware/CLAUDE.md](CLAUDE.md) | [firmware/notes/](notes/) | [notes/architecture.md](/notes/architecture.md)

## Contents

- [Changelog](#changelog)
- [Principles](#principles)
- [Dual-Pulse PWM Architecture](#dual-pulse-pwm-architecture)
- [Magic Header](#magic-header)
- [Commands — app → ESP32](#commands-app--esp32)
  - [set](#set-channel-parameters)
  - [off](#turn-off-one-channel)
  - [alloff](#turn-off-all-channels)
  - [status](#request-full-status)
  - [preset](#apply-a-named-preset)
  - [profile upload](#upload-a-profile)
  - [run](#run-a-stored-profile)
  - [stop](#stop-a-running-profile)
  - [listprofiles](#list-stored-profiles)
  - [deleteprofile](#delete-a-stored-profile)
- [Responses — ESP32 → app](#responses-esp32--app)
- [Telemetry — ESP32 → app unprompted](#telemetry-esp32--app-unprompted)
- [Sensor Bus](#sensor-bus)
- [Profile Step Structure](#profile-step-structure)
- [Error Reference](#error-reference)
- [GPIO Assignments](#gpio-assignments)
- [PWM Implementation Notes](#pwm-implementation-notes)
- [Version](#version)

---

## Changelog
- v1.0 April 2026 — initial schema, PSU phase
- v1.1 April 2026 — added dual-pulse PWM architecture, carrier and switch frequency as control parameters, updated profile step structure, updated set command

---

## Principles

- Every message in both directions carries `"magic":"refine"` — discard anything without it
- Commands flow app → ESP32, responses flow ESP32 → app
- Telemetry flows ESP32 → app unprompted, periodically
- Errors return the error key only — no ok/true redundancy
- All sensor names and response vectors are generic strings — adding new hardware never breaks existing messages
- Profiles are state machines: each step runs until its exit condition is satisfied
- Never remove or rename existing fields — extend by adding new ones alongside

---

## Dual-Pulse PWM Architecture

Each channel runs two independent 100kHz PWM signals (duty_a and duty_b) switched at a lower frequency (freq_switch, default 1kHz). The cell's double-layer capacitance integrates the 100kHz carrier into two average voltage levels, giving a shaped low-frequency waveform without any output filter components.

```
freq_carrier = 100kHz   duty_a = 80%  →  ~9.6V average during phase A
freq_carrier = 100kHz   duty_b = 20%  →  ~2.4V average during phase B
freq_switch  = 1kHz     →  alternates between phase A and phase B
```

For simple DC-equivalent operation set duty_a = duty_b.
For single-level PWM set freq_switch = 0 and use duty_a only.

This is implemented entirely in the ESP32 LEDC peripheral — no additional hardware required.

---

## Magic Header

Every message, both directions, must contain:

```json
{"magic":"refine", ...}
```

ESP32 silently discards any message missing this field.

---

## Commands (app → ESP32)

### Set channel parameters
```json
{"magic":"refine", "cmd":"set", "ch":1,
  "duty_a":80, "duty_b":20,
  "freq_carrier":100000, "freq_switch":1000}
```
- `ch`: 1, 2, or 3
- `duty_a`: 0–100 (percent) — high phase duty cycle
- `duty_b`: 0–100 (percent) — low phase duty cycle
- `freq_carrier`: Hz — carrier PWM frequency (default 100000)
- `freq_switch`: Hz — switching frequency between phases (default 1000, 0 = duty_a only)

Simple operation shorthand — omitted fields use current or default values:
```json
{"magic":"refine", "cmd":"set", "ch":1, "duty_a":65, "duty_b":65}
```

### Turn off one channel
```json
{"magic":"refine", "cmd":"off", "ch":1}
```

### Turn off all channels
```json
{"magic":"refine", "cmd":"alloff"}
```

### Request full status
```json
{"magic":"refine", "cmd":"status"}
```

### Apply a named preset
```json
{"magic":"refine", "cmd":"preset", "process":"copper", "ch":1}
```
- Preset values are owned by the app and sent as `set` commands
- Process name is informational — used for logging and display only

### Upload a profile
```json
{"magic":"refine", "cmd":"profile", "ch":1, "name":"silver_deplate_v1", "steps":[
  {
    "label": "fast strip",
    "duty_a": 80,
    "duty_b": 60,
    "freq_carrier": 100000,
    "freq_switch": 1000,
    "until": {"sensor":"cell_v", "op":"gt", "value":2.1},
    "alert_if": {"sensor":"cell_a", "op":"gt", "value":9.0}
  },
  {
    "label": "taper",
    "duty_a": 40,
    "duty_b": 20,
    "freq_carrier": 100000,
    "freq_switch": 500,
    "until": {"sensor":"cell_a", "op":"lt", "value":0.3},
    "alert_if": null
  },
  {
    "label": "final cleanup",
    "duty_a": 10,
    "duty_b": 10,
    "freq_carrier": 100000,
    "freq_switch": 0,
    "until": {"sensor":"cell_a", "op":"lt", "value":0.1},
    "alert_if": null
  },
  {
    "label": "done",
    "duty_a": 0,
    "duty_b": 0,
    "freq_carrier": 100000,
    "freq_switch": 0,
    "until": null,
    "alert_if": null
  }
]}
```
- ESP32 stores and runs the profile autonomously
- Phone does not need to stay connected during a run
- `until: null` on final step means hold until manually stopped
- `alert_if` triggers an alert event without stopping the step — null means no alert condition

### Run a stored profile
```json
{"magic":"refine", "cmd":"run", "ch":1, "name":"silver_deplate_v1"}
```

### Stop a running profile
```json
{"magic":"refine", "cmd":"stop", "ch":1}
```

### List stored profiles
```json
{"magic":"refine", "cmd":"listprofiles"}
```

### Delete a stored profile
```json
{"magic":"refine", "cmd":"deleteprofile", "name":"silver_deplate_v1"}
```

---

## Responses (ESP32 → app)

### Acknowledge set / off / preset
```json
{"magic":"refine", "ch":1, "duty_a":80, "duty_b":20,
  "freq_carrier":100000, "freq_switch":1000}
```

### Full status response
```json
{"magic":"refine", "status":[
  {"ch":1, "duty_a":80, "duty_b":20, "freq_carrier":100000, "freq_switch":1000,
   "running":false, "profile":null},
  {"ch":2, "duty_a":0,  "duty_b":0,  "freq_carrier":100000, "freq_switch":1000,
   "running":false, "profile":null},
  {"ch":3, "duty_a":30, "duty_b":10, "freq_carrier":100000, "freq_switch":500,
   "running":true,  "profile":"silver_deplate_v1"}
]}
```

### Profile list response
```json
{"magic":"refine", "profiles":[
  "silver_deplate_v1",
  "copper_plate_std",
  "zinc_strip_v2"
]}
```

### Profile upload confirmed
```json
{"magic":"refine", "profile_saved":"silver_deplate_v1"}
```

### Error response
```json
{"magic":"refine", "error":"bad channel"}
{"magic":"refine", "error":"bad duty"}
{"magic":"refine", "error":"bad freq"}
{"magic":"refine", "error":"bad command"}
{"magic":"refine", "error":"bad process"}
{"magic":"refine", "error":"bad profile"}
{"magic":"refine", "error":"profile not found"}
{"magic":"refine", "error":"overcurrent", "ch":1}
{"magic":"refine", "error":"sensor fault", "sensor":"cell_v", "ch":1}
```

---

## Telemetry (ESP32 → app, unprompted)

### Per-channel sensor telemetry (~1/sec)
```json
{"magic":"refine", "telem":true, "ch":1, "sensors":{
  "cell_v": 1.8,
  "cell_a": 4.2
}}
```

### Profile step change event
```json
{"magic":"refine", "telem":true, "ch":1, "event":"step_change",
  "profile":"silver_deplate_v1", "step":1, "label":"taper",
  "duty_a":40, "duty_b":20}
```

### Profile complete event
```json
{"magic":"refine", "telem":true, "ch":1, "event":"profile_complete",
  "profile":"silver_deplate_v1"}
```

### Alert event
```json
{"magic":"refine", "telem":true, "ch":1, "event":"alert",
  "reason":"overcurrent"}
```

---

## Sensor Bus

Sensors are identified by name strings. Adding a new sensor requires no schema changes.

### Current sensor names
| Name | Description |
|---|---|
| `cell_v` | Cell voltage |
| `cell_a` | Cell current |

### Reserved for future use
| Name | Description |
|---|---|
| `ph` | Electrolyte pH |
| `temp` | Electrolyte temperature |
| `cell_r` | Calculated cell resistance (v/a) — derived, not physical sensor |

---

## Profile Step Structure

| Field | Type | Description |
|---|---|---|
| `label` | string | Human readable step name for UI and logging |
| `duty_a` | 0–100 | High phase PWM duty cycle |
| `duty_b` | 0–100 | Low phase PWM duty cycle |
| `freq_carrier` | Hz | Carrier frequency (default 100000) |
| `freq_switch` | Hz | Phase switching frequency (default 1000, 0 = duty_a only) |
| `until` | object or null | Exit condition — null means hold indefinitely |
| `until.sensor` | string | Sensor name from sensor bus |
| `until.op` | string | Comparison: `gt`, `lt`, `gte`, `lte`, `eq` |
| `until.value` | number | Threshold value |
| `alert_if` | object or null | Condition that triggers alert without stopping step |

### Reserved future fields
| Field | Description |
|---|---|
| `waypoint` | Named checkpoint — app shows progress against it |
| `response_vector` | Action beyond duty — trigger pump, open valve etc |

---

## Error Reference

| Error string | Meaning |
|---|---|
| `bad channel` | ch not 1, 2, or 3 |
| `bad duty` | duty_a or duty_b outside 0–100 |
| `bad freq` | freq_carrier or freq_switch outside supported range |
| `bad command` | cmd string not recognised |
| `bad process` | process name not recognised |
| `bad profile` | profile JSON malformed or missing required fields |
| `profile not found` | named profile not in storage |
| `overcurrent` | current exceeded safe threshold on channel |
| `sensor fault` | sensor returning implausible or no value |

---

## GPIO Assignments

| Function | GPIO |
|---|---|
| Channel 1 PWM | 25 |
| Channel 2 PWM | 26 |
| Channel 3 PWM | 27 |
| Status LED | 2 |

---

## PWM Implementation Notes

- ESP32 LEDC peripheral handles all PWM generation
- Two independent timers per channel — one for each phase
- 1kHz ISR in firmware switches active phase output
- At 100kHz carrier the cell double-layer capacitance integrates pulses into average voltage
- No output filter components required

---

## Version

| Field | Value |
|---|---|
| Schema version | 1.1 |
| Date | April 2026 |
| Status | Draft — PSU phase |
| Next review | When voltage/current sensing added |
