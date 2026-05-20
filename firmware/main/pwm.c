/*
 * pwm.c — ESP32 LEDC PWM output and dual-phase switching
 *
 * Manages three channels (GPIO 25/26/27). Each channel has two duty cycles
 * (duty_a, duty_b) switched by a gptimer ISR at freq_switch (default 1 kHz).
 * Setting duty_a == duty_b or freq_switch == 0 gives simple single-level PWM.
 *
 * Key invariants:
 *   - Resolution is 8-bit (LEDC_TIMER_8_BIT, 0–255). Do NOT change to 10-bit —
 *     100 kHz at 10-bit crashes ledc_timer_config (see CLAUDE.md bug #2)
 *   - ch_idx is 0-based internally; protocol "ch" field is 1-based
 *   - ISR (switch_timer_cb) uses IRAM_ATTR and direct register access —
 *     must not call any flash-cached functions
 *   - s_mux spinlock guards all fields read by both tasks and the ISR
 *   - Known limitation: switch timer runs at a fixed 1 kHz regardless of
 *     freq_switch value; per-channel switching rates are a future task
 *
 * Related:
 *   firmware/main/pwm.h             — public API and channel_state_t struct
 *   firmware/notes/hardware.md      — GPIO assignments and verification checklist
 *   firmware/CLAUDE.md              — PWM bug history
 */
#include "pwm.h"

#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "soc/ledc_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define TAG "REFINE"

// GPIO per channel (0-based index)
static const int CH_GPIO[NUM_CHANNELS] = {25, 26, 27};

// LEDC channel assignment — all HIGH_SPEED_MODE
static const ledc_channel_t CH_LEDC[NUM_CHANNELS] = {
    LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2
};

// Independent LEDC timers so each channel can have its own freq_carrier
static const ledc_timer_t CH_TIMER[NUM_CHANNELS] = {
    LEDC_TIMER_0, LEDC_TIMER_1, LEDC_TIMER_2
};

static channel_state_t s_ch[NUM_CHANNELS];
static gptimer_handle_t s_switch_timer = NULL;

// Spinlock shared between tasks and the switch-timer ISR
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static inline uint32_t percent_to_ledc(int pct) {
    return (uint32_t)(((uint32_t)pct * LEDC_MAX_DUTY) / 100);
}

/*
 * ISR-safe LEDC duty write.
 * Uses direct register access (IRAM-safe, no flash dependency).
 * channel_group[0] = LEDC_HIGH_SPEED_MODE.
 * The duty register stores the integer count left-shifted by 4
 * (four fractional bits per ESP32 TRM).
 */
static inline void IRAM_ATTR ledc_duty_set_fast(int ch_num, uint32_t duty_counts) {
    LEDC.channel_group[0].channel[ch_num].duty.duty = duty_counts << 4;
    LEDC.channel_group[0].channel[ch_num].conf1.duty_start = 1;
}

// ---------------------------------------------------------------------------
// 1 kHz switch-timer ISR
// ---------------------------------------------------------------------------
/*
 * Fires at freq_switch (currently fixed 1 kHz).  Each call toggles the
 * active duty cycle on every channel that has switching enabled.
 *
 * NOTE: This implementation drives a fixed 1 kHz toggle rate.  Channels
 * configured with freq_switch != 1000 will switch at the wrong rate in
 * this version.  Independent per-channel timers are required to support
 * arbitrary switching frequencies — deferred to a future session.
 */
static bool IRAM_ATTR switch_timer_cb(gptimer_handle_t timer,
                                       const gptimer_alarm_event_data_t *edata,
                                       void *user_ctx) {
    (void)timer;
    (void)edata;
    (void)user_ctx;

    portENTER_CRITICAL_ISR(&s_mux);
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (s_ch[i].freq_switch == 0) {
            continue;
        }
        s_ch[i].phase = !s_ch[i].phase;
        uint32_t duty = s_ch[i].phase ? s_ch[i].ledc_duty_b : s_ch[i].ledc_duty_a;
        ledc_duty_set_fast(i, duty);
    }
    portEXIT_CRITICAL_ISR(&s_mux);

    return false; // no higher-priority task woken
}

// ---------------------------------------------------------------------------
// Static init helpers
// ---------------------------------------------------------------------------

static void configure_ledc_timer(int ch_idx, int freq_carrier) {
    ledc_timer_config_t tcfg = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = CH_TIMER[ch_idx],
        .freq_hz          = (uint32_t)freq_carrier,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));
}

static void configure_ledc_channel(int ch_idx) {
    ledc_channel_config_t ccfg = {
        .gpio_num   = CH_GPIO[ch_idx],
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel    = CH_LEDC[ch_idx],
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = CH_TIMER[ch_idx],
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void pwm_init(void) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        s_ch[i].ch           = i + 1;
        s_ch[i].duty_a       = 0;
        s_ch[i].duty_b       = 0;
        s_ch[i].freq_carrier = 100000;
        s_ch[i].freq_switch  = 1000;
        s_ch[i].running      = false;
        s_ch[i].ledc_duty_a  = 0;
        s_ch[i].ledc_duty_b  = 0;
        s_ch[i].phase        = false;

        configure_ledc_timer(i, 100000);
        configure_ledc_channel(i);
    }

    // 1 kHz hardware switch timer (1 MHz tick source → alarm every 1000 ticks)
    gptimer_config_t gt_cfg = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&gt_cfg, &s_switch_timer));

    gptimer_alarm_config_t alarm_cfg = {
        .reload_count = 0,
        .alarm_count  = 1000,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(s_switch_timer, &alarm_cfg));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = switch_timer_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(s_switch_timer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(s_switch_timer));
    ESP_ERROR_CHECK(gptimer_start(s_switch_timer));

    ESP_LOGI(TAG, "PWM init: 3 channels, 100 kHz carrier, 8-bit resolution, 1 kHz switch timer");
}

void pwm_set_channel(int ch_idx, int duty_a, int duty_b,
                     int freq_carrier, int freq_switch) {
    if (ch_idx < 0 || ch_idx >= NUM_CHANNELS) return;

    // Reconfigure LEDC timer only if carrier frequency changed
    if (freq_carrier != s_ch[ch_idx].freq_carrier) {
        configure_ledc_timer(ch_idx, freq_carrier);
    }

    uint32_t la = percent_to_ledc(duty_a);
    uint32_t lb = percent_to_ledc(duty_b);

    portENTER_CRITICAL(&s_mux);
    s_ch[ch_idx].duty_a       = duty_a;
    s_ch[ch_idx].duty_b       = duty_b;
    s_ch[ch_idx].freq_carrier = freq_carrier;
    s_ch[ch_idx].freq_switch  = freq_switch;
    s_ch[ch_idx].running      = (duty_a > 0 || duty_b > 0);
    s_ch[ch_idx].ledc_duty_a  = la;
    s_ch[ch_idx].ledc_duty_b  = lb;
    s_ch[ch_idx].phase        = false;  // restart at phase A
    portEXIT_CRITICAL(&s_mux);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, CH_LEDC[ch_idx], la);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, CH_LEDC[ch_idx]);
}

void pwm_off_channel(int ch_idx) {
    if (ch_idx < 0 || ch_idx >= NUM_CHANNELS) return;

    portENTER_CRITICAL(&s_mux);
    s_ch[ch_idx].duty_a      = 0;
    s_ch[ch_idx].duty_b      = 0;
    s_ch[ch_idx].freq_switch = 0;
    s_ch[ch_idx].running     = false;
    s_ch[ch_idx].ledc_duty_a = 0;
    s_ch[ch_idx].ledc_duty_b = 0;
    s_ch[ch_idx].phase       = false;
    portEXIT_CRITICAL(&s_mux);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, CH_LEDC[ch_idx], 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, CH_LEDC[ch_idx]);
}

void pwm_all_off(void) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        pwm_off_channel(i);
    }
}

channel_state_t *pwm_get_state(int ch_idx) {
    if (ch_idx < 0 || ch_idx >= NUM_CHANNELS) return NULL;
    return &s_ch[ch_idx];
}
