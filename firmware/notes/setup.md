# Firmware Build Environment Setup

For build commands and flash commands, see [CLAUDE.md](../CLAUDE.md) — this file only covers initial machine setup.

## What you need

- Linux machine (Lubuntu confirmed working)
- Python 3
- USB-UART adapter connected to ESP32
- ESP-IDF v5.5

## Install ESP-IDF v5.5

```bash
# Install prerequisites
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone ESP-IDF v5.5 into home directory
git clone --recursive --branch v5.5 https://github.com/espressif/esp-idf.git ~/esp-idf-v5.5

# Run the installer (downloads toolchain, sets up Python venv)
cd ~/esp-idf-v5.5
./install.sh esp32
```

## Install Python serial tools (for monitor and esptool)

```bash
pip3 install pyserial esptool
```

## USB permissions (so you don't need sudo for /dev/ttyUSB0)

```bash
sudo usermod -aG dialout $USER
# Log out and back in for this to take effect
```

## Verify setup

```bash
ls /dev/ttyUSB*                  # should show ttyUSB0 or ttyUSB1
bash -c "source ~/esp-idf-v5.5/export.sh && idf.py --version"
```

## Get the code

```bash
git clone https://github.com/andrew1976fell/refineps.git
cd refineps/firmware
```

## Build

```bash
bash -c "source ~/esp-idf-v5.5/export.sh && idf.py build"
```

Then flash using the esptool command in [CLAUDE.md](../CLAUDE.md) — do not use `idf.py flash`.
