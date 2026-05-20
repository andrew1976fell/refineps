# Hardware Notes

## Board

- ESP32 (2MB flash, BLE only — Classic BT disabled in sdkconfig)
- USB-UART adapter on `/dev/ttyUSB0` or `/dev/ttyUSB1`

## GPIO assignments

| GPIO | Function | Status |
|------|----------|--------|
| 25 | Channel 1 PWM output | Not yet verified with meter |
| 26 | Channel 2 PWM output | Not yet verified with meter |
| 27 | Channel 3 PWM output | Not yet verified with meter |
| 2  | Status LED | Assumed — not tested |

## PWM architecture

- Carrier frequency: 100 kHz
- Resolution: **8-bit (0–255)** — do not change, see [CLAUDE.md](../CLAUDE.md) bug #2
- Two independent phases per channel (duty_a and duty_b)
- Phase switching frequency configurable per channel (default 1 kHz, 0 = duty_a only)
- Implemented via ESP32 LEDC peripheral — no external filter components needed
- The cell's double-layer capacitance integrates the 100 kHz carrier into an average voltage

## Sensor inputs

| Sensor | GPIO | Status |
|--------|------|--------|
| cell_v (cell voltage) | TBD | Not wired — firmware returns dummy 0.0 |
| cell_a (cell current) | TBD | Not wired — firmware returns dummy 0.0 |

ADC sensing is the next major firmware task after GPIO verification.

## Hardware verification checklist

- [ ] GPIO 25: probe with meter while `hw_test.py` runs — confirm PWM voltage levels at each duty cycle step
- [ ] GPIO 26: same
- [ ] GPIO 27: same
- [ ] Confirm 8-bit duty cycle maps correctly to expected voltage (e.g. duty 128 ≈ 50% of supply)
- [ ] Wire cell_v ADC input and implement real reading in `telemetry.c`
- [ ] Wire cell_a ADC input and implement real reading in `telemetry.c`

## Running the hardware test

Connect meter to GPIO 25, then:

```bash
python3 ~/Downloads/refine-firmware/firmware/hw_test.py
```

The script connects over BLE and steps through channels with 8-second pauses — enough time to read the meter at each step.
