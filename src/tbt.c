#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "ble2mqtt/ble2mqtt.h"
#include "tbt.h"

#define INVALID_HANDLE 0

static const char *TAG = "tbt";
static ble2mqtt_t *ble2mqtt = NULL;
static const uint32_t scan_duration = UINT32_MAX;
static bool is_scanning = false;

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static bledevice_t *bt_get_dev_by_address(esp_bd_addr_t address);

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x400,
    .scan_window = 0x30,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
    },
};

static void bt_scan_start()
{
    if (is_scanning)
        return;

    ESP_LOGD(TAG, "Start scanning");
    esp_ble_gap_start_scanning(scan_duration);
    is_scanning = true;
}

static void bt_scan_stop()
{
    if (!is_scanning)
        return;

    ESP_LOGD(TAG, "Stop scanning");
    esp_ble_gap_stop_scanning();
    is_scanning = false;
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    {
        ESP_LOGD(TAG, "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT");
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
    }
    break;

    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    {
        ESP_LOGD(TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT");
        bt_scan_start();
    }
    break;

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    {
        ESP_LOGD(TAG, "ESP_GAP_BLE_SCAN_START_COMPLETE_EVT");

        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Scan start success");
        }
        else
        {
            ESP_LOGE(TAG, "Scan start failed");
        }
    }
    break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt)
        {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
        {
            bledevice_t *dev = bt_get_dev_by_address(scan_result->scan_rst.bda);
            if (!dev)
                break;
            if (dev->is_connected)
                break;
            if (dev->is_connecting)
                break;

            bt_scan_stop();

            ESP_LOGI(TAG, "Try to open connection to: %s. Address type: %d", dev->address_str, dev->address_type);
            esp_err_t ret = esp_ble_gattc_open(dev->gattc_if, dev->address, dev->address_type, true);
            if (ret)
            {
                ESP_LOGE(TAG, "open connection to %s failed, error code = %x", dev->address_str, ret);
            }
            dev->is_connecting = true;
        }
        break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
    }
    break;

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    {
        ESP_LOGD(TAG, "ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT");

        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(TAG, "Stop scan successfully");
        is_scanning = false;
    }
    break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    {
        ESP_LOGD(TAG, "ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT");

        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(TAG, "Stop adv successfully");
    }
    break;

    default:
        ESP_LOGD(TAG, "Got event: %d", event);
        break;
    }
}

/**
 * Search bt device by address
 * @return pointer to bledevice_t, or NULL if device not found
 */
static bledevice_t *bt_get_dev_by_address(esp_bd_addr_t address)
{
    for (int i = 0; i < ble2mqtt->devices_len; i++)
        if (memcmp(address, ble2mqtt->devices[i]->address, ESP_BD_ADDR_LEN) == 0)
            return ble2mqtt->devices[i];

    return NULL;
}

/**
 * Search bt device by connection id
 * @return pointer to bledevice_t, or NULL if device not found
 */
static bledevice_t *bt_get_dev_by_conn_id(uint16_t conn_id)
{
    for (int i = 0; i < ble2mqtt->devices_len; i++)
        if (conn_id == ble2mqtt->devices[i]->conn_id)
            return ble2mqtt->devices[i];

    return NULL;
}

/**
 * Search bt device by gattc_if
 * @return pointer to bledevice_t, or NULL if device not found
 */
static bledevice_t *bt_get_dev_by_gattc_if(esp_gatt_if_t gattc_if)
{
    for (int i = 0; i < ble2mqtt->devices_len; i++)
        if (gattc_if == ble2mqtt->devices[i]->gattc_if)
            return ble2mqtt->devices[i];

    return NULL;
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(TAG, "EVT %d, gattc if %d", event, gattc_if);
    bledevice_t *dev = NULL;
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event)
    {
    case ESP_GATTC_REG_EVT: //0
        ESP_LOGD(TAG, "REG_EVT");
        if (p_data->reg.status == ESP_GATT_OK)
        {
            ble2mqtt->devices[p_data->reg.app_id]->gattc_if = gattc_if;
        }
        else
        {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                     p_data->reg.app_id,
                     p_data->reg.status);
            return;
        }
        break;

    case ESP_GATTC_OPEN_EVT: //2
        ESP_LOGD(TAG, "OPEN_EVT");
        if (p_data->open.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "open failed, status %d", p_data->open.status);
            break;
        }

        dev = bt_get_dev_by_address(p_data->open.remote_bda);
        if (dev)
        {
            dev->is_connecting = false;
            dev->is_connected = true;
            dev->conn_id = p_data->cfg_mtu.conn_id;

            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, p_data->open.conn_id);
            if (mtu_ret)
            {
                ESP_LOGE(TAG, "config MTU error, error code = %x", mtu_ret);
            }
        }
        break;

    case ESP_GATTC_SEARCH_CMPL_EVT: //6
    {
        ESP_LOGD(TAG, "SEARCH_CMPL_EVT");

        if (p_data->search_cmpl.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }

        uint16_t count = 0;
        dev = bt_get_dev_by_conn_id(p_data->search_cmpl.conn_id);
        if (!dev)
            break;

        esp_gatt_status_t status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                p_data->search_cmpl.conn_id,
                                                                ESP_GATT_DB_CHARACTERISTIC,
                                                                dev->service_start_handle,
                                                                dev->service_end_handle,
                                                                INVALID_HANDLE,
                                                                &count);
        if (status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error. ErrNo: %d", status);
        }

        if (count > 0)
        {
            dev->char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
            if (!dev->char_elem_result)
            {
                ESP_LOGE(TAG, "gattc no mem");
            }
            else
            {
                status = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                                        p_data->search_cmpl.conn_id,
                                                        dev->service_start_handle,
                                                        dev->service_end_handle,
                                                        dev->char_uuid,
                                                        dev->char_elem_result,
                                                        &count);
                if (status != ESP_GATT_OK)
                {
                    ESP_LOGE(TAG, "esp_ble_gattc_get_char_by_uuid error. ErrNo: %d", status);
                }

                /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                if (count > 0 && (dev->char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
                {
                    dev->char_handle = dev->char_elem_result[0].char_handle;
                    esp_ble_gattc_register_for_notify(gattc_if, dev->address, dev->char_elem_result[0].char_handle);
                }
            }
            /* free char_elem_result */
            free(dev->char_elem_result);
            dev->char_elem_result = NULL;
        }
        else
        {
            ESP_LOGE(TAG, "no char found");
        }
    }
    break;

    case ESP_GATTC_SEARCH_RES_EVT: //7
    {
        ESP_LOGD(TAG, "SEARCH_RES_EVT");
        dev = bt_get_dev_by_conn_id(p_data->search_res.conn_id);
        if (!dev)
            break;

        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.len == dev->service_uuid.len && p_data->search_res.srvc_id.uuid.uuid.uuid16 == dev->service_uuid.uuid.uuid16)
        {
            dev->service_start_handle = p_data->search_res.start_handle;
            dev->service_end_handle = p_data->search_res.end_handle;
        }
        else if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_32 && p_data->search_res.srvc_id.uuid.len == dev->service_uuid.len && p_data->search_res.srvc_id.uuid.uuid.uuid32 == dev->service_uuid.uuid.uuid32)
        {
            dev->service_start_handle = p_data->search_res.start_handle;
            dev->service_end_handle = p_data->search_res.end_handle;
        }
        else if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128 && p_data->search_res.srvc_id.uuid.len == dev->service_uuid.len)
        {
            dev->service_start_handle = p_data->search_res.start_handle;
            dev->service_end_handle = p_data->search_res.end_handle;
        }
    }
    break;

    case ESP_GATTC_WRITE_DESCR_EVT: //9
    {
        ESP_LOGD(TAG, "WRITE_DESCR_EVT");
        if (p_data->write.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "Write description error");
        }
    }
    break;

    case ESP_GATTC_NOTIFY_EVT: //10
    {
        ESP_LOGD(TAG, "NOTIFY_EVT");

        if (p_data->notify.is_notify)
            ESP_LOGI(TAG, "Got notify");
        else
            ESP_LOGI(TAG, "Got indicate");
        esp_log_buffer_hex(TAG, p_data->notify.value, p_data->notify.value_len);

        btle_q_element_t *elem = malloc(sizeof(btle_q_element_t));
        if (!elem)
        {
            ESP_LOGE(TAG, "malloc error!");
            break;
        }
        memcpy(elem->address, p_data->notify.remote_bda, ESP_BD_ADDR_LEN);
        elem->is_notify = p_data->notify.is_notify;
        elem->value_len = p_data->notify.value_len;
        elem->value = malloc(sizeof(elem->value_len));
        if (!elem->value)
        {
            ESP_LOGE(TAG, "malloc error!");
            break;
        }
        memcpy(elem->value, p_data->notify.value, elem->value_len);
        xQueueSend(ble2mqtt->xQueue, (void *)&elem, (TickType_t)20);
        xEventGroupSetBits(ble2mqtt->s_event_group, BLE2MQTT_NEW_BTLE_MESSAGE);
    }
    break;

    case ESP_GATTC_CFG_MTU_EVT: //18
    {
        ESP_LOGD(TAG, "CFG_MTU_EVT");
        if (p_data->cfg_mtu.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "Config mtu failed");
        }

        dev = bt_get_dev_by_conn_id(p_data->cfg_mtu.conn_id);
        if (dev)
        {
            ESP_LOGI(TAG, "Status %d, MTU %d, conn_id %d", p_data->cfg_mtu.status, p_data->cfg_mtu.mtu, p_data->cfg_mtu.conn_id);
            esp_ble_gattc_search_service(gattc_if, p_data->cfg_mtu.conn_id, &dev->service_uuid);
        }
    }
    break;

    case ESP_GATTC_REG_FOR_NOTIFY_EVT: //38
    {
        ESP_LOGI(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");

        if (p_data->reg_for_notify.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            break;
        }

        dev = bt_get_dev_by_gattc_if(gattc_if);
        if (!dev)
        {
            ESP_LOGE(TAG, "Device not found!");
            break;
        }
        uint16_t count = 0;
        uint16_t notify_en = 1;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                    dev->conn_id,
                                                                    ESP_GATT_DB_DESCRIPTOR,
                                                                    dev->service_start_handle,
                                                                    dev->service_end_handle,
                                                                    dev->char_handle,
                                                                    &count);
        if (ret_status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error");
        }
        if (count > 0)
        {
            dev->descr_elem_result = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!dev->descr_elem_result)
            {
                ESP_LOGE(TAG, "malloc error, gattc no mem");
            }
            else
            {
                ret_status = esp_ble_gattc_get_descr_by_char_handle(gattc_if,
                                                                    dev->conn_id,
                                                                    p_data->reg_for_notify.handle,
                                                                    notify_descr_uuid,
                                                                    dev->descr_elem_result,
                                                                    &count);
                if (ret_status != ESP_GATT_OK)
                {
                    ESP_LOGE(TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                }

                /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                if (count > 0 && dev->descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && dev->descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG)
                {
                    ret_status = esp_ble_gattc_write_char_descr(gattc_if,
                                                                dev->conn_id,
                                                                dev->descr_elem_result[0].handle,
                                                                sizeof(notify_en),
                                                                (uint8_t *)&notify_en,
                                                                ESP_GATT_WRITE_TYPE_RSP,
                                                                ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK)
                {
                    ESP_LOGE(TAG, "esp_ble_gattc_write_char_descr error");
                }

                /* free descr_elem_result */
                free(dev->descr_elem_result);
            }
        }
        else
        {
            ESP_LOGE(TAG, "decsr not found");
        }
    }
    break;

    case ESP_GATTC_CONNECT_EVT: //40
        ESP_LOGD(TAG, "CONNECT_EVT");
        break;

    case ESP_GATTC_DISCONNECT_EVT: //41
        ESP_LOGD(TAG, "DISCONNECT_EVT");
        dev = bt_get_dev_by_address(p_data->disconnect.remote_bda);
        if (dev)
        {
            dev->is_connecting = false;
            dev->is_connected = false;
            dev->conn_id = 0xFFFF;
        }

        switch (p_data->disconnect.reason)
        {
        case ESP_GATT_CONN_UNKNOWN:
            ESP_LOGD(TAG, "ESP_GATT_CONN_UNKNOWN");
            break;
        case ESP_GATT_CONN_L2C_FAILURE:
            ESP_LOGD(TAG, "ESP_GATT_CONN_L2C_FAILURE");
            break;
        case ESP_GATT_CONN_TIMEOUT:
            ESP_LOGD(TAG, "ESP_GATT_CONN_TIMEOUT");
            break;
        case ESP_GATT_CONN_TERMINATE_PEER_USER:
            ESP_LOGD(TAG, "ESP_GATT_CONN_TERMINATE_PEER_USER");
            break;
        case ESP_GATT_CONN_TERMINATE_LOCAL_HOST:
            ESP_LOGD(TAG, "ESP_GATT_CONN_TERMINATE_LOCAL_HOST");
            break;
        case ESP_GATT_CONN_FAIL_ESTABLISH:
            ESP_LOGD(TAG, "ESP_GATT_CONN_FAIL_ESTABLISH");
            break;
        case ESP_GATT_CONN_LMP_TIMEOUT:
            ESP_LOGD(TAG, "ESP_GATT_CONN_LMP_TIMEOUT");
            break;
        case ESP_GATT_CONN_CONN_CANCEL:
            ESP_LOGD(TAG, "ESP_GATT_CONN_CONN_CANCEL");
            break;
        case ESP_GATT_CONN_NONE:
            ESP_LOGD(TAG, "ESP_GATT_CONN_NONE");
            break;
        }

        bt_scan_start();
        break;

    case ESP_GATTC_DIS_SRVC_CMPL_EVT: //46
        ESP_LOGD(TAG, "DIS_SRVC_CMPL_EVT");
        if (p_data->dis_srvc_cmpl.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "discover service failed, status %d", p_data->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGE(TAG, "discover service complete conn_id %d, errNo: %d", p_data->dis_srvc_cmpl.conn_id, p_data->dis_srvc_cmpl.status);
        break;

    default:
        ESP_LOGD(TAG, "Unsupported event: %d", event);
        break;
    }
}

void vTaskBt(void *pvParameters)
{
    ESP_LOGI(TAG, "Run task bluetooth");
    ble2mqtt = (ble2mqtt_t *)pvParameters;
    EventBits_t bits;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret)
    {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if (ret)
    {
        ESP_LOGE(TAG, "gattc register error, error code = %x", ret);
        return;
    }

    for (int i = 0; i < BTL2MQTT_DEV_MAX_LEN; i++)
    {
        ret = esp_ble_gattc_app_register(i);
        if (ret)
        {
            ESP_LOGE(TAG, "gattc app register error, error code = %x", ret);
            return;
        }
    }

    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret)
    {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", ret);
    }

    while (1)
    {
        //#ifndef DISABLETASKMSG
        ESP_LOGD(TAG, "Task bt. is_scanning: %s", (is_scanning == true ? "Y" : "N"));
        //#endif

        if (!(xEventGroupGetBits(ble2mqtt->s_event_group) & BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT))
        {
            ESP_LOGD(TAG, "Wait for device list");
            bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT, pdFALSE, pdFALSE, portMAX_DELAY); //Wait for device list
            if (!(bits & BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT))
                continue;
            ESP_LOGD(TAG, "Got device list. Set scan params and start scan.");

            ret = esp_ble_gap_set_scan_params(&ble_scan_params);
            if (ret)
            {
                ESP_LOGE(TAG, "set scan params error, error code = %x", ret);
            }
        }

        if (!is_scanning)
        {
            for (int i = 0; i < ble2mqtt->devices_len; i++)
            {
                if (!ble2mqtt->devices[i]->is_connected || !ble2mqtt->devices[i]->is_connecting)
                {
                    bt_scan_start();
                    break;
                }
            }
        }

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}