/*
 * schema.c — JSON command dispatch and response formatting
 *
 * Runs as a FreeRTOS task (schema_task). Receives heap char* JSON strings
 * from the bt_serial message queue, parses them with cJSON, dispatches to
 * the appropriate handler, sends a response via bt_serial_write_json, then
 * frees the message.
 *
 * Implemented commands: set, off, alloff, status
 * Not yet implemented:  profile, run, stop, listprofiles, deleteprofile
 *   (all return "bad command" for now — see TODOs in dispatch block)
 *
 * Key invariants:
 *   - Messages missing "magic":"refine" are discarded silently — no response
 *   - Malformed JSON is discarded silently — no response
 *   - bt_serial_write_json() deletes the cJSON tree; do not use root after calling it
 *   - ch in protocol is 1-based; ch_idx passed to pwm_* is 0-based
 *
 * Related:
 *   firmware/main/schema.h          — task entry point declaration
 *   firmware/refine_schema_v1.1.md  — full command/response protocol
 *   firmware/main/bt_serial.h       — write API used for responses
 *   firmware/main/pwm.h             — channel control called by handlers
 */
#include "schema.h"
#include "bt_serial.h"
#include "pwm.h"

#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <string.h>
#include <stdlib.h>

#define TAG "REFINE"

// ---------------------------------------------------------------------------
// Response helpers
// ---------------------------------------------------------------------------

static void send_error(const char *msg) {
    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "magic", "refine");
    cJSON_AddStringToObject(r, "error", msg);
    bt_serial_write_json(r);
}

static void send_channel_state(int ch_idx) {
    channel_state_t *s = pwm_get_state(ch_idx);
    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "magic",        "refine");
    cJSON_AddNumberToObject(r, "ch",           s->ch);
    cJSON_AddNumberToObject(r, "duty_a",       s->duty_a);
    cJSON_AddNumberToObject(r, "duty_b",       s->duty_b);
    cJSON_AddNumberToObject(r, "freq_carrier", s->freq_carrier);
    cJSON_AddNumberToObject(r, "freq_switch",  s->freq_switch);
    bt_serial_write_json(r);
}

static void send_full_status(void) {
    cJSON *r   = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddStringToObject(r, "magic", "refine");

    for (int i = 0; i < NUM_CHANNELS; i++) {
        channel_state_t *s  = pwm_get_state(i);
        cJSON           *ch = cJSON_CreateObject();
        cJSON_AddNumberToObject(ch, "ch",           s->ch);
        cJSON_AddNumberToObject(ch, "duty_a",       s->duty_a);
        cJSON_AddNumberToObject(ch, "duty_b",       s->duty_b);
        cJSON_AddNumberToObject(ch, "freq_carrier", s->freq_carrier);
        cJSON_AddNumberToObject(ch, "freq_switch",  s->freq_switch);
        cJSON_AddBoolToObject  (ch, "running",      s->running);
        cJSON_AddNullToObject  (ch, "profile");     // TODO: profile storage (future session)
        cJSON_AddItemToArray(arr, ch);
    }

    cJSON_AddItemToObject(r, "status", arr);
    bt_serial_write_json(r);
}

// ---------------------------------------------------------------------------
// Command handlers
// ---------------------------------------------------------------------------

static void handle_set(cJSON *root) {
    cJSON *ch_item = cJSON_GetObjectItemCaseSensitive(root, "ch");
    if (!cJSON_IsNumber(ch_item)) { send_error("bad channel"); return; }
    int ch = (int)ch_item->valuedouble;
    if (ch < 1 || ch > NUM_CHANNELS) { send_error("bad channel"); return; }
    int ch_idx = ch - 1;

    // Missing fields default to current channel values
    channel_state_t *s = pwm_get_state(ch_idx);
    int duty_a       = s->duty_a;
    int duty_b       = s->duty_b;
    int freq_carrier = s->freq_carrier;
    int freq_switch  = s->freq_switch;

    cJSON *da = cJSON_GetObjectItemCaseSensitive(root, "duty_a");
    if (cJSON_IsNumber(da)) duty_a = (int)da->valuedouble;

    cJSON *db = cJSON_GetObjectItemCaseSensitive(root, "duty_b");
    if (cJSON_IsNumber(db)) duty_b = (int)db->valuedouble;

    cJSON *fc = cJSON_GetObjectItemCaseSensitive(root, "freq_carrier");
    if (cJSON_IsNumber(fc)) freq_carrier = (int)fc->valuedouble;

    cJSON *fs = cJSON_GetObjectItemCaseSensitive(root, "freq_switch");
    if (cJSON_IsNumber(fs)) freq_switch = (int)fs->valuedouble;

    // Validate
    if (duty_a < 0 || duty_a > 100 || duty_b < 0 || duty_b > 100) {
        send_error("bad duty");
        return;
    }
    if (freq_carrier < 1000 || freq_carrier > 200000) {
        send_error("bad freq");
        return;
    }
    if (freq_switch < 0 || freq_switch > 10000) {
        send_error("bad freq");
        return;
    }

    pwm_set_channel(ch_idx, duty_a, duty_b, freq_carrier, freq_switch);
    send_channel_state(ch_idx);

    ESP_LOGI(TAG, "set ch%d: duty_a=%d duty_b=%d fc=%d fs=%d",
             ch, duty_a, duty_b, freq_carrier, freq_switch);
}

static void handle_off(cJSON *root) {
    cJSON *ch_item = cJSON_GetObjectItemCaseSensitive(root, "ch");
    if (!cJSON_IsNumber(ch_item)) { send_error("bad channel"); return; }
    int ch = (int)ch_item->valuedouble;
    if (ch < 1 || ch > NUM_CHANNELS) { send_error("bad channel"); return; }
    int ch_idx = ch - 1;

    pwm_off_channel(ch_idx);
    send_channel_state(ch_idx);

    ESP_LOGI(TAG, "off ch%d", ch);
}

static void handle_alloff(void) {
    pwm_all_off();

    cJSON *r = cJSON_CreateObject();
    cJSON_AddStringToObject(r, "magic", "refine");
    cJSON_AddBoolToObject(r, "ok", cJSON_True);
    bt_serial_write_json(r);

    ESP_LOGI(TAG, "alloff");
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

static void schema_process(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        // Malformed JSON — discard silently
        return;
    }

    // Require magic:"refine" — discard silently if missing or wrong
    cJSON *magic = cJSON_GetObjectItemCaseSensitive(root, "magic");
    if (!cJSON_IsString(magic) || strcmp(magic->valuestring, "refine") != 0) {
        cJSON_Delete(root);
        return;
    }

    cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    if (!cJSON_IsString(cmd)) {
        // No cmd field — not a command message; discard silently
        cJSON_Delete(root);
        return;
    }

    const char *cmd_str = cmd->valuestring;

    if (strcmp(cmd_str, "set") == 0) {
        handle_set(root);
    } else if (strcmp(cmd_str, "off") == 0) {
        handle_off(root);
    } else if (strcmp(cmd_str, "alloff") == 0) {
        handle_alloff();
    } else if (strcmp(cmd_str, "status") == 0) {
        send_full_status();
    } else {
        // Unrecognised command — includes future commands (profile, run, stop …)
        // TODO (future session): implement profile, run, stop, listprofiles, deleteprofile
        send_error("bad command");
        ESP_LOGW(TAG, "Unknown cmd: %s", cmd_str);
    }

    cJSON_Delete(root);
}

// ---------------------------------------------------------------------------
// Task entry point
// ---------------------------------------------------------------------------

void schema_task(void *pvParameters) {
    QueueHandle_t q = (QueueHandle_t)pvParameters;
    char *msg;

    ESP_LOGI(TAG, "Schema task started");

    while (1) {
        if (xQueueReceive(q, &msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGD(TAG, "RX: %s", msg);
            schema_process(msg);
            free(msg);
        }
    }
}
