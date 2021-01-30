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

static const char *TAG = "tbt";
static ble2mqtt_t *ble2mqtt = NULL;
static const uint32_t scan_duration = 60 * 30; //30 min
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

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(TAG, "EVT %d, gattc if %d", event, gattc_if);
    bledevice_t *dev = NULL;

    switch (event)
    {
    case ESP_GATTC_REG_EVT: //0
        ESP_LOGD(TAG, "REG_EVT");
        if (param->reg.status == ESP_GATT_OK)
        {
            ble2mqtt->devices[param->reg.app_id]->gattc_if = gattc_if;
        }
        else
        {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }

        break;

    case ESP_GATTC_OPEN_EVT: //2
        ESP_LOGD(TAG, "OPEN_EVT");
        if (param->open.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "open failed, status %d", param->open.status);
        }
        break;

    case ESP_GATTC_CONNECT_EVT: //40
        ESP_LOGD(TAG, "CONNECT_EVT");
        dev = bt_get_dev_by_address(param->connect.remote_bda);
        if (dev)
        {
            dev->is_connecting = false;
            dev->is_connected = true;
        }
        break;

    case ESP_GATTC_DISCONNECT_EVT: //41
        ESP_LOGD(TAG, "DISCONNECT_EVT");
        dev = bt_get_dev_by_address(param->disconnect.remote_bda);
        if (dev)
        {
            dev->is_connecting = false;
            dev->is_connected = false;
        }

        switch (param->disconnect.reason)
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
#ifndef DISABLETASKMSG
        ESP_LOGD(TAG, "Task bt");
#endif

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