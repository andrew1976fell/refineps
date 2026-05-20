# Workflow — working from any machine

## Starting from a library or new PC (no local files needed)

1. Open a browser — go to github.com/andrew1976fell/refineps
2. Sign in with GitHub account: `andrew1976fell` / email: `andrew1967fell@gmail.com`
3. Clone the repo:
   ```
   git clone https://github.com/andrew1976fell/refineps.git
   ```
4. Firmware work needs Lubuntu with ESP-IDF installed — not possible on a library PC
5. Android work needs Android Studio — not possible on a library PC
6. For reading/editing notes and code: use the GitHub web editor directly (no install needed)

## Lubuntu (home machine) — firmware dev

```bash
# Build
bash -c "source ~/esp-idf-v5.5/export.sh && idf.py build"

# Flash (idf.py flash does NOT work on this machine — use esptool directly)
# See firmware/CLAUDE.md for the exact esptool command

# Monitor (passive, no DTR reset)
python3 /tmp/read_serial.py

# BLE test
python3 ~/Downloads/refine-firmware/hw_test.py
```

Local clone: `~/Downloads/refine-firmware`  
Remote: `https://github.com/andrew1976fell/refineps.git`

## Windows (work machine) — Android dev

1. Open terminal in repo folder
2. `git pull`
3. Android Studio → **File → Open** → select the `android/` subfolder (not the repo root)

## Push changes

```bash
git add -p          # stage selectively
git commit -m "..."
git push
```

## Rule

Never keep work-in-progress only on a local machine.
Commit and push before closing the laptop.
