/*
 * pwm.h — public API and shared types for PWM channel control
 *
 * channel_state_t holds both the protocol-facing values (duty_a/b as percent)
 * and the ISR-facing pre-computed LEDC counts (ledc_duty_a/b). The volatile
 * ISR fields must not be reordered — the switch_timer_cb ISR reads them
 * without holding the spinlock after the duty values are pre-computed.
 *
 * ch_idx arguments are 0-based (0–2); protocol "ch" field is 1-based (1–3).
 *
 * Related: firmware/main/pwm.c
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NUM_CHANNELS      3
#define LEDC_MAX_DUTY     255    // 8-bit: (1<<8)-1

typedef struct {
    int      ch;            // 1-based channel number
    int      duty_a;        // percent 0-100 (stored value)
    int      duty_b;        // percent 0-100 (stored value)
    int      freq_carrier;  // Hz
    int      freq_switch;   // Hz (0 = duty_a only, no switching)
    bool     running;       // true if duty_a > 0 or duty_b > 0

    // ISR-accessed fields — do not reorder
    volatile uint32_t ledc_duty_a;  // pre-computed LEDC counts for phase A
    volatile uint32_t ledc_duty_b;  // pre-computed LEDC counts for phase B
    volatile bool     phase;        // false = A active, true = B active
} channel_state_t;

void             pwm_init(void);
void             pwm_set_channel(int ch_idx, int duty_a, int duty_b,
                                 int freq_carrier, int freq_switch);
void             pwm_off_channel(int ch_idx);
void             pwm_all_off(void);
channel_state_t *pwm_get_state(int ch_idx);
