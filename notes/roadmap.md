# Roadmap

Priority order across the whole project. Update this when priorities shift.

## In progress

- [ ] Android app: BLE connect and basic channel control UI

## Up next

- [ ] Hardware verification: GPIO 25/26/27 PWM levels (run `hw_test.py` with meter)
- [ ] Android: send set/off/alloff/status commands from UI
- [ ] Firmware: real ADC sensing — cell_v and cell_a (currently dummy 0.0)

## Later

- [ ] Profile system: define, upload, run, stop, list, delete (firmware + Android)
- [ ] Persist profiles to NVS flash so they survive reboot
- [ ] Android: profile editor UI

## Done

- [x] BLE connects and all commands respond (status, set, off, alloff, telemetry)
- [x] Repo restructured: firmware/ and android/ under one GitHub repo
- [x] Android app: BLE fragmentation fixed, Gradle updated
