#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "cJSON.h"
#include <stdbool.h>

/* Initialise BLE GATT server. msg_queue receives heap char* command strings. */
void bt_serial_init(QueueHandle_t msg_queue);

/* Send a null-terminated string as a BLE notification. Thread-safe. */
void bt_serial_write(const char *data);

/*
 * Serialise root to compact JSON, send as BLE notification, delete the tree.
 * Caller must not use root after this call.
 */
void bt_serial_write_json(cJSON *root);

/* Returns true while a client is connected. */
bool bt_serial_is_connected(void);
