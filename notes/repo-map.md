# Repo Map

What every folder and file in the repo root is for.

## Top-level structure

```
refineps/
├── firmware/          ESP32 BLE firmware (ESP-IDF / C)
├── android/           Android app (Kotlin / Jetpack Compose)
├── notes/             Cross-project notes (this folder)
├── build/             Generated build output — not committed, gitignored
├── managed_components/ ESP-IDF component manager cache — not committed
└── .gitignore         Combined ignore rules for firmware and android builds
```

## firmware/

| Path | Purpose |
|------|---------|
| `firmware/main/` | All C source files — see [firmware/notes/source-map.md](/firmware/notes/source-map.md) |
| `firmware/CLAUDE.md` | **Authoritative** — flash commands, build rules, known bugs |
| `firmware/refine_schema_v1.1.md` | **Authoritative** — full BLE protocol spec |
| `firmware/flash.sh` | esptool flash convenience script |
| `firmware/restore.sh` | Restores a known-good build state |
| `firmware/hw_test.py` | BLE test tool — steps through all channels |
| `firmware/ble_cmd.py` | Send a single BLE command from the command line |
| `firmware/hw_test_power.py` | Power-focused hardware test |
| `firmware/sdkconfig` | Generated ESP-IDF build config — committed so builds are reproducible |
| `firmware/sdkconfig.defaults` | Source-controlled defaults applied before sdkconfig is generated |
| `firmware/notes/` | Firmware-specific notes: source map, hardware, setup |

## android/

| Path | Purpose |
|------|---------|
| `android/app/` | Android app module source |
| `android/app/build.gradle.kts` | App-level build config (SDK versions, dependencies) |
| `android/build.gradle.kts` | Project-level build config |
| `android/settings.gradle.kts` | Project name and module declarations |
| `android/gradle/` | Gradle wrapper config |
| `android/notes/` | Android-specific notes: setup, BLE, UI |

## notes/

| File | Purpose |
|------|---------|
| `README.md` | Index — links to authoritative docs and component notes |
| `repo-map.md` | This file |
| `roadmap.md` | All outstanding work, priority ordered |
| `architecture.md` | System diagram, BLE UUIDs, command summary |
| `workflow.md` | How to work from any machine including a library PC |
| `decisions.md` | Key technical choices and why |

## GitHub

Repo: `https://github.com/andrew1976fell/refineps`  
Account: `andrew1976fell`
