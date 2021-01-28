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

#define APP_ID_0 0

static const char *TAG = "tbt";
static ble2mqtt_t *ble2mqtt = NULL;

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    ESP_LOGI(TAG, "Got new BT event: %d", event);
    switch (event)
    {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    {
        //the unit of the duration is second
        uint32_t duration = 30;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Scan start success");
        }
        else
        {
            ESP_LOGE(TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt)
        {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(TAG, adv_name, adv_name_len);
            ESP_LOGI(TAG, "\n");
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(TAG, "Stop scan successfully");

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    ESP_LOGI(TAG, "EVT %d, gattc if %d", event, gattc_if);

    if (event == ESP_GATTC_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            ble2mqtt->bt.gattc_if = gattc_if;
            xEventGroupSetBits(ble2mqtt->s_event_group, BLE2MQTT_BT_GOT_GATT_IF_BIT);
        }
    }
    else
    {
        ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                 param->reg.app_id,
                 param->reg.status);
        return;
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

    ret = esp_ble_gattc_app_register(APP_ID_0);
    if (ret)
    {
        ESP_LOGE(TAG, "gattc app register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatt_set_local_mtu(200);
    if (ret)
    {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", ret);
    }

    while (1)
    {
        ESP_LOGD(TAG, "Task bt");
        if (!(xEventGroupGetBits(ble2mqtt->s_event_group) & BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT))
        {
            bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT, pdFALSE, pdFALSE, portMAX_DELAY); //Wait for device list
            if (!(bits & BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT))
                continue;
        }

        if (!(xEventGroupGetBits(ble2mqtt->s_event_group) & BLE2MQTT_BT_GOT_GATT_IF_BIT))
        {
            bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_BT_GOT_GATT_IF_BIT, pdFALSE, pdFALSE, portMAX_DELAY); //Wait for gatt interface
            if (!(bits & BLE2MQTT_BT_GOT_GATT_IF_BIT))
                continue;
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}