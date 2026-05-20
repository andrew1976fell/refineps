/*
 * main.c — app_main entry point
 *
 * CURRENT STATE: hardware verification stub.
 * Drives all three PWM channels at 100% and loops forever.
 * BLE, schema, and telemetry are compiled in but NOT called from here.
 *
 * To run the full BLE firmware stack, app_main needs to:
 *   1. Create the message queue
 *   2. Call bt_serial_init(queue)
 *   3. Launch schema_task on the queue via xTaskCreate
 *   4. Call telemetry_init()
 *
 * Related:
 *   firmware/main/bt_serial.h       — BLE GATT server init and write API
 *   firmware/main/schema.h          — JSON command dispatch task
 *   firmware/main/telemetry.h       — 1 Hz telemetry timer
 *   firmware/notes/source-map.md    — role of each source file
 */
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "TEST100"

static const int            GPIOS[3]    = {25, 26, 27};
static const ledc_channel_t CHANNELS[3] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2};
static const ledc_timer_t   TIMERS[3]   = {LEDC_TIMER_0,   LEDC_TIMER_1,   LEDC_TIMER_2};

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "100%% PWM test — GPIO 25 / 26 / 27");

    for (int i = 0; i < 3; i++) {
        ledc_timer_config_t tcfg = {
            .speed_mode      = LEDC_HIGH_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .timer_num       = TIMERS[i],
            .freq_hz         = 100000,
            .clk_cfg         = LEDC_AUTO_CLK,
        };
        ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

        ledc_channel_config_t ccfg = {
            .gpio_num   = GPIOS[i],
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .channel    = CHANNELS[i],
            .intr_type  = LEDC_INTR_DISABLE,
            .timer_sel  = TIMERS[i],
            .duty       = 255,
            .hpoint     = 0,
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ccfg));
    }

    ESP_LOGI(TAG, "All 3 channels → 100%%  Probe GPIO 25 / 26 / 27");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
