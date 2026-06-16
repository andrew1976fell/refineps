/*
 * main.c — app_main entry point
 *
 * Brings up the full BLE firmware stack. app_main:
 *   1. Initialises NVS (required by the BLE stack)
 *   2. Creates the message queue (heap char* JSON command strings)
 *   3. Initialises the PWM peripheral (channel state telemetry reads)
 *   4. Calls bt_serial_init(queue) to start the BLE GATT server
 *   5. Launches schema_task on the queue via xTaskCreate
 *   6. Calls telemetry_init() to start the 1 Hz telemetry timer
 *      (after pwm_init() and bt_serial_init(), per telemetry.h)
 *
 * Related:
 *   firmware/main/bt_serial.h       — BLE GATT server init and write API
 *   firmware/main/pwm.h             — LEDC PWM channel control and state
 *   firmware/main/schema.h          — JSON command dispatch task
 *   firmware/main/telemetry.h       — 1 Hz telemetry timer
 *   firmware/notes/source-map.md    — role of each source file
 */
#include "bt_serial.h"
#include "pwm.h"
#include "schema.h"
#include "telemetry.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG             "REFINE"
#define MSG_QUEUE_DEPTH 16

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, ">>> app_main() entered");

    // 1. NVS — required by the BLE stack.
    ESP_LOGI(TAG, ">>> nvs_flash_init");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Message queue — bt_serial posts heap char* commands, schema_task frees them.
    ESP_LOGI(TAG, ">>> xQueueCreate");
    QueueHandle_t msg_queue = xQueueCreate(MSG_QUEUE_DEPTH, sizeof(char *));
    if (!msg_queue) {
        ESP_LOGE(TAG, "Failed to create message queue");
        return;
    }

    // 3. PWM peripheral — telemetry reads channel state, so init before it.
    ESP_LOGI(TAG, ">>> pwm_init");
    pwm_init();

    // 4. BLE GATT server.
    ESP_LOGI(TAG, ">>> bt_serial_init");
    bt_serial_init(msg_queue);

    // 5. JSON command dispatch task — consumes the queue.
    ESP_LOGI(TAG, ">>> xTaskCreate schema");
    xTaskCreate(schema_task, "schema", 4096, (void *)msg_queue, 10, NULL);

    // 6. Telemetry — uses the BLE write API on first tick, so start it last.
    ESP_LOGI(TAG, ">>> telemetry_init");
    telemetry_init();

    ESP_LOGI(TAG, ">>> All tasks launched — ready");
}
