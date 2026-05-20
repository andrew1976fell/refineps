/*
 * telemetry.c — periodic sensor telemetry over BLE
 *
 * Starts a 1 Hz esp_timer that sends a JSON telemetry message for each
 * channel where running == true (duty_a > 0 || duty_b > 0). Only sends
 * while a BLE client is connected.
 *
 * Current state: cell_v and cell_a are dummy 0.0 constants.
 * Real ADC sensing is a future task — replace the hardcoded values in
 * send_channel_telem with actual esp_adc_cal reads when the ADC is wired.
 *
 * Related:
 *   firmware/main/telemetry.h       — public init API
 *   firmware/main/bt_serial.h       — bt_serial_is_connected, bt_serial_write_json
 *   firmware/main/pwm.h             — pwm_get_state, channel_state_t
 *   firmware/refine_schema_v1.1.md  — telemetry message format
 *   firmware/notes/hardware.md      — ADC wiring status and next steps
 */
#include "telemetry.h"
#include "bt_serial.h"
#include "pwm.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"

#define TAG              "REFINE"
#define TELEM_INTERVAL_US (1000 * 1000)  // 1 second

/*
 * Send one telemetry message for the given channel.
 * Sensor values are dummy constants — real ADC integration is a future session.
 */
static void send_channel_telem(int ch_idx) {
    channel_state_t *s = pwm_get_state(ch_idx);

    cJSON *t       = cJSON_CreateObject();
    cJSON *sensors = cJSON_CreateObject();

    cJSON_AddStringToObject(t, "magic", "refine");
    cJSON_AddBoolToObject  (t, "telem", true);
    cJSON_AddNumberToObject(t, "ch",    s->ch);

    // TODO (future session): replace with real ADC readings
    cJSON_AddNumberToObject(sensors, "cell_v", 0.0);
    cJSON_AddNumberToObject(sensors, "cell_a", 0.0);

    cJSON_AddItemToObject(t, "sensors", sensors);
    bt_serial_write_json(t);
}

static void telemetry_timer_cb(void *arg) {
    (void)arg;

    if (!bt_serial_is_connected()) {
        return;
    }

    for (int i = 0; i < NUM_CHANNELS; i++) {
        channel_state_t *s = pwm_get_state(i);
        if (s->running) {
            send_channel_telem(i);
        }
    }
}

void telemetry_init(void) {
    esp_timer_handle_t timer;
    esp_timer_create_args_t args = {
        .callback        = telemetry_timer_cb,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "telem",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, TELEM_INTERVAL_US));

    ESP_LOGI(TAG, "Telemetry timer started (1 Hz)");
}
