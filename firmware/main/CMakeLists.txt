# main/CMakeLists.txt — source files and IDF component dependencies
#
# Add new .c files to SRCS when adding source files to the project.
# Add new IDF components to REQUIRES when using a new ESP-IDF API.
# After any change here, rebuild (fullclean not required for this file alone):
#   bash -c "source ~/esp-idf-v5.5/export.sh && idf.py build"
#
# SRCS — what each file does:
#   main.c        entry point, initialises BLE, PWM, telemetry in order
#   bt_serial.c   BLE GATT server, fragment reassembly, send/receive JSON
#   pwm.c         ESP32 LEDC peripheral, dual-phase PWM, channel state
#   schema.c      JSON command parsing, validation, dispatch, responses
#   telemetry.c   1-second timer task, sensor readings, BLE notify
#
# REQUIRES — why each component is needed:
#   nvs_flash         non-volatile storage (future profile persistence)
#   bt                Bluetooth stack (BLE GATT server)
#   esp_driver_ledc   PWM output via LEDC peripheral
#   esp_driver_gptimer general-purpose timer (dual-phase PWM switching)
#   esp_driver_gpio   GPIO control (status LED)
#   esp_timer         high-resolution timer (telemetry 1-second tick)
#   freertos          RTOS tasks and queues
#   esp_hw_support    hardware abstraction layer
#   soc               SoC-level constants and capabilities
#
# Related:
#   firmware/notes/source-map.md    — fuller description of each source file
#   firmware/CMakeLists.txt         — top-level project file
#   firmware/CLAUDE.md              — build commands and rules

idf_component_register(
    SRCS
        "main.c"
        "bt_serial.c"
        "pwm.c"
        "schema.c"
        "telemetry.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        bt
        esp_driver_ledc
        esp_driver_gptimer
        esp_driver_gpio
        esp_timer
        freertos
        esp_hw_support
        soc
)
