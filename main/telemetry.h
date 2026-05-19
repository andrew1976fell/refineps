#pragma once

/*
 * Start the 1-second telemetry timer.
 * Call after pwm_init() and bt_serial_init() are both complete.
 */
void telemetry_init(void);
