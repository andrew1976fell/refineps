#include "bt_serial.h"
#include "pwm.h"
#include "schema.h"
#include "telemetry.h"

#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG            "REFINE"
#define GPIO_STATUS_LED  2
#define MSG_QUEUE_DEPTH 16

// ---------------------------------------------------------------------------
// Status LED task
// ---------------------------------------------------------------------------
/*
 * Blink pattern:
 *   BT not connected          → slow 1 Hz (500 ms on / 500 ms off)
 *   BT connected, all off     → steady on
 *   BT connected, any active  → fast 5 Hz (100 ms on / 100 ms off)
 */
static void led_task(void *arg) {
    (void)arg;
    gpio_reset_pin(GPIO_STATUS_LED);
    gpio_set_direction(GPIO_STATUS_LED, GPIO_MODE_OUTPUT);

    while (1) {
        bool connected  = bt_serial_is_connected();
        bool any_active = false;

        for (int i = 0; i < NUM_CHANNELS; i++) {
            channel_state_t *s = pwm_get_state(i);
            if (s->running) { any_active = true; break; }
        }

        if (!connected) {
            // Slow 1 Hz blink
            gpio_set_level(GPIO_STATUS_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(GPIO_STATUS_LED, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (any_active) {
            // Fast 5 Hz blink
            gpio_set_level(GPIO_STATUS_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(GPIO_STATUS_LED, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            // Steady on — re-evaluate every 100 ms to catch state changes
            gpio_set_level(GPIO_STATUS_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// ---------------------------------------------------------------------------
// App entry point
// ---------------------------------------------------------------------------

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, ">>> app_main() entered");

    ESP_LOGI(TAG, ">>> nvs_flash_init");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, ">>> nvs_flash_erase (partition dirty)");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, ">>> nvs_flash_init OK");

    ESP_LOGI(TAG, ">>> xQueueCreate");
    QueueHandle_t msg_queue = xQueueCreate(MSG_QUEUE_DEPTH, sizeof(char *));
    if (!msg_queue) {
        ESP_LOGE(TAG, "Failed to create message queue");
        return;
    }
    ESP_LOGI(TAG, ">>> xQueueCreate OK");

    ESP_LOGI(TAG, ">>> pwm_init");
    pwm_init();
    ESP_LOGI(TAG, ">>> pwm_init OK");

    ESP_LOGI(TAG, ">>> bt_serial_init");
    bt_serial_init(msg_queue);
    ESP_LOGI(TAG, ">>> bt_serial_init OK");

    ESP_LOGI(TAG, ">>> telemetry_init");
    telemetry_init();
    ESP_LOGI(TAG, ">>> telemetry_init OK");

    ESP_LOGI(TAG, ">>> xTaskCreate schema");
    xTaskCreate(schema_task, "schema", 4096, (void *)msg_queue, 10, NULL);
    ESP_LOGI(TAG, ">>> xTaskCreate schema OK");

    ESP_LOGI(TAG, ">>> xTaskCreate led");
    xTaskCreate(led_task, "led", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, ">>> xTaskCreate led OK");

    ESP_LOGI(TAG, ">>> All tasks launched — ready");
}
