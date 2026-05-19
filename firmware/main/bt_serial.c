#include "bt_serial.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "cJSON.h"

#include <string.h>
#include <stdlib.h>

#define TAG           "REFINE"
#define DEVICE_NAME   "RefinePS"
#define LINE_BUF_SIZE 512
#define GATTS_APP_ID  0

// Custom 16-bit UUIDs
#define SVC_UUID      0x00FF
#define CHAR_RX_UUID  0xFF01   // client → device: commands (WRITE)
#define CHAR_TX_UUID  0xFF02   // device → client: responses & telemetry (NOTIFY)

// Attribute table indices
enum {
    IDX_SVC,
    IDX_CHAR_RX,
    IDX_CHAR_RX_VAL,
    IDX_CHAR_TX,
    IDX_CHAR_TX_VAL,
    IDX_CHAR_TX_CCCD,
    IDX_NUM,
};

static uint16_t         s_handle_table[IDX_NUM];
static uint16_t         s_conn_id        = 0xFFFF;
static uint16_t         s_gatts_if       = ESP_GATT_IF_NONE;
static volatile bool    s_connected      = false;
static volatile bool    s_notify_enabled = false;
static uint16_t         s_mtu            = 23;

static QueueHandle_t     s_msg_queue   = NULL;
static SemaphoreHandle_t s_write_mutex = NULL;


// ---------------------------------------------------------------------------
// Attribute table
// ---------------------------------------------------------------------------

static const uint16_t uuid_primary_service = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t uuid_char_decl       = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t uuid_char_cfg        = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t prop_write  = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t prop_notify = ESP_GATT_CHAR_PROP_BIT_READ  | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint16_t svc_uuid     = SVC_UUID;
static const uint16_t char_rx_uuid = CHAR_RX_UUID;
static const uint16_t char_tx_uuid = CHAR_TX_UUID;

static const uint8_t char_rx_val[1] = {0};
static const uint8_t char_tx_val[1] = {0};
static       uint8_t cccd_val[2]    = {0x00, 0x00};

static const esp_gatts_attr_db_t s_gatt_db[IDX_NUM] = {
    [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&uuid_primary_service,
         ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(svc_uuid), (uint8_t *)&svc_uuid}
    },
    [IDX_CHAR_RX] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&uuid_char_decl,
         ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(prop_write), (uint8_t *)&prop_write}
    },
    [IDX_CHAR_RX_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_rx_uuid,
         ESP_GATT_PERM_WRITE,
         LINE_BUF_SIZE, sizeof(char_rx_val), (uint8_t *)char_rx_val}
    },
    [IDX_CHAR_TX] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&uuid_char_decl,
         ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(prop_notify), (uint8_t *)&prop_notify}
    },
    [IDX_CHAR_TX_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_tx_uuid,
         ESP_GATT_PERM_READ,
         LINE_BUF_SIZE, sizeof(char_tx_val), (uint8_t *)char_tx_val}
    },
    [IDX_CHAR_TX_CCCD] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&uuid_char_cfg,
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(uint16_t), sizeof(cccd_val), (uint8_t *)cccd_val}
    },
};

// ---------------------------------------------------------------------------
// Advertising
// ---------------------------------------------------------------------------

static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .appearance          = 0,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,
    .p_service_uuid      = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min       = 0x20,
    .adv_int_max       = 0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// ---------------------------------------------------------------------------
// GAP callback
// ---------------------------------------------------------------------------

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param) {
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&s_adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "BLE advertising start failed");
        } else {
            ESP_LOGI(TAG, "BLE advertising as \"%s\"", DEVICE_NAME);
        }
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// GATTS callback
// ---------------------------------------------------------------------------

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
    switch (event) {

    case ESP_GATTS_REG_EVT:
        if (param->reg.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "GATTS app register failed: %d", param->reg.status);
            break;
        }
        s_gatts_if = gatts_if;
        esp_ble_gap_set_device_name(DEVICE_NAME);
        esp_ble_gap_config_adv_data(&s_adv_data);
        esp_ble_gatts_create_attr_tab(s_gatt_db, gatts_if, IDX_NUM, 0);
        break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Attribute table create failed: %d",
                     param->add_attr_tab.status);
            break;
        }
        if (param->add_attr_tab.num_handle != IDX_NUM) {
            ESP_LOGE(TAG, "Attribute table handle count mismatch: %d vs %d",
                     param->add_attr_tab.num_handle, IDX_NUM);
            break;
        }
        memcpy(s_handle_table, param->add_attr_tab.handles,
               sizeof(s_handle_table));
        esp_ble_gatts_start_service(s_handle_table[IDX_SVC]);
        break;

    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "GATT service started");
        break;

    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "BLE client connected");
        s_conn_id        = param->connect.conn_id;
        s_connected      = true;
        s_notify_enabled = false;
        {
            // Request stable connection parameters: 20-40ms interval,
            // no slave latency, 6s supervision timeout (0x258 = 600 * 10ms).
            esp_ble_conn_update_params_t conn_params = {
                .min_int  = 0x10,   // 20ms
                .max_int  = 0x20,   // 40ms
                .latency  = 0,
                .timeout  = 0x0258, // 6s
            };
            memcpy(conn_params.bda, param->connect.remote_bda,
                   sizeof(esp_bd_addr_t));
            esp_ble_gap_update_conn_params(&conn_params);
        }
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "BLE client disconnected — restarting advertising");
        s_connected      = false;
        s_notify_enabled = false;
        s_conn_id        = 0xFFFF;
        s_mtu            = 23;
        esp_ble_gap_start_advertising(&s_adv_params);
        break;

    case ESP_GATTS_MTU_EVT:
        s_mtu = param->mtu.mtu;
        ESP_LOGI(TAG, "MTU negotiated: %u", s_mtu);
        break;

    case ESP_GATTS_WRITE_EVT: {
        uint16_t handle = param->write.handle;

        if (handle == s_handle_table[IDX_CHAR_RX_VAL]) {
            int len = param->write.len;
            ESP_LOGI(TAG, "RX write: %d bytes", len);
            ESP_LOG_BUFFER_HEX(TAG, param->write.value, (uint16_t)len);
            if (len > 0 && len < LINE_BUF_SIZE) {
                char *msg = malloc((size_t)(len + 1));
                if (msg) {
                    memcpy(msg, param->write.value, (size_t)len);
                    msg[len] = '\0';
                    if (xQueueSend(s_msg_queue, &msg, 0) != pdTRUE) {
                        ESP_LOGW(TAG, "msg_queue full — dropping command");
                        free(msg);
                    }
                }
            }
        } else if (handle == s_handle_table[IDX_CHAR_TX_CCCD]) {
            if (param->write.len == 2) {
                uint16_t cccd = (uint16_t)((param->write.value[1] << 8) |
                                            param->write.value[0]);
                s_notify_enabled = (cccd & 0x0001) != 0;
                ESP_LOGI(TAG, "Notifications %s",
                         s_notify_enabled ? "enabled" : "disabled");
            }
        }

        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                        param->write.trans_id,
                                        ESP_GATT_OK, NULL);
        }
        break;
    }

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void bt_serial_init(QueueHandle_t msg_queue) {
    s_msg_queue   = msg_queue;
    s_write_mutex = xSemaphoreCreateMutex();

    // Classic BT is not compiled in (CONFIG_BT_CLASSIC_ENABLED=n), nothing to release.
    // Calling esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT) here returns
    // ESP_ERR_NOT_SUPPORTED in IDF v5.4+ and causes abort() via ESP_ERROR_CHECK.

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(GATTS_APP_ID));
    ESP_ERROR_CHECK(esp_ble_gatt_set_local_mtu(517));

    ESP_LOGI(TAG, "BLE GATT server init done — waiting for client");
}

void bt_serial_write(const char *data) {
    if (!s_connected || !s_notify_enabled || !data) return;
    size_t len = strlen(data);
    if (len == 0) return;

    uint16_t max_payload = (s_mtu > 3) ? (uint16_t)(s_mtu - 3) : 20;

    if (xSemaphoreTake(s_write_mutex, pdMS_TO_TICKS(200)) != pdTRUE) return;

    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > max_payload) chunk = max_payload;
        esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id,
                                    s_handle_table[IDX_CHAR_TX_VAL],
                                    (uint16_t)chunk,
                                    (uint8_t *)(data + offset), false);
        offset += chunk;
    }

    xSemaphoreGive(s_write_mutex);
}

void bt_serial_write_json(cJSON *root) {
    char *str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!str) return;
    bt_serial_write(str);
    free(str);
}

bool bt_serial_is_connected(void) {
    return s_connected;
}
