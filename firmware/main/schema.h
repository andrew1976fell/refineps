/*
 * schema.h — public API for JSON command dispatch task
 *
 * Pass to xTaskCreate; pvParameters must be a QueueHandle_t that receives
 * heap-allocated char* JSON command strings from bt_serial. This task owns
 * and frees each message after processing.
 *
 * Related: firmware/main/schema.c
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/*
 * FreeRTOS task entry point.
 * pvParameters must be a QueueHandle_t from which char* messages are received.
 * Each message is a heap-allocated null-terminated JSON string; this task
 * frees it after processing.
 */
void schema_task(void *pvParameters);
