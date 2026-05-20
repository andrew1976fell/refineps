# Notes

High-level notes that span the whole RefinePS project (firmware + Android app + any future components).
Anything that belongs to a single component lives in that component's own folder.

## Authoritative references (read these before anything else)

| File | Purpose |
|------|---------|
| [firmware/CLAUDE.md](/firmware/CLAUDE.md) | Flash commands, build rules, known bugs — do not contradict this |
| [firmware/refine_schema_v1.1.md](/firmware/refine_schema_v1.1.md) | Full BLE protocol — all commands, responses, telemetry, error codes |

## Cross-project notes (this folder)

| File | Purpose |
|------|---------|
| [repo-map.md](repo-map.md) | Repo structure — what every top-level folder and file is for |
| [roadmap.md](roadmap.md) | What needs building, in priority order |
| [architecture.md](architecture.md) | How all the pieces fit together |
| [workflow.md](workflow.md) | How to work across machines — setup steps for a new/library PC |
| [decisions.md](decisions.md) | Key technical decisions and why they were made |

## Component notes

| Folder | Purpose |
|--------|---------|
| [firmware/notes/](/firmware/notes/) | Firmware-specific: source map, hardware verification, build environment setup |
| [android/notes/](/android/notes/) | Android-specific: Android Studio setup, BLE implementation, UI structure |

## Rule

One truth. If something is written here, it is not also written elsewhere.
Update here first, then update code/config to match.
