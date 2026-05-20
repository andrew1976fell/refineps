/*
 * telemetry.h — public API for the 1 Hz telemetry timer
 *
 * Call telemetry_init() once from app_main after both pwm_init() and
 * bt_serial_init() are complete — it reads channel state and uses the
 * BLE write API immediately on first tick.
 *
 * Related: firmware/main/telemetry.c
 */
#pragma once

/*
 * Start the 1-second telemetry timer.
 * Call after pwm_init() and bt_serial_init() are both complete.
 */
void telemetry_init(void);
