#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_gattc_api.h"

#include "ble2mqtt.h"

static const char *TAG = "ble2mqtt";

ble2mqtt_t *ble2mqtt_create(void)
{
    ble2mqtt_t *ble2mqtt = malloc(sizeof(ble2mqtt_t));

    if (!ble2mqtt)
        return NULL;

    ble2mqtt->xQueue = xQueueCreate(10, sizeof(btle_q_element_t *));

    if (!ble2mqtt->xQueue)
        return NULL;

    return ble2mqtt;
}

bool ble2mqtt_init(ble2mqtt_t *ble2mqtt)
{
    ESP_LOGI(TAG, "Init ble2mqtt_t");

    ble2mqtt->s_event_group = xEventGroupCreate();
    ble2mqtt->xMutexDevices = xSemaphoreCreateMutex();
    if (!ble2mqtt->xMutexDevices)
        return false;

    ble2mqtt->devices_len = 0;

    for (int i = 0; i < BTL2MQTT_DEV_MAX_LEN; i++)
    {
        ble2mqtt->devices[i] = malloc(sizeof(bledevice_t));
        if (!ble2mqtt->devices[i])
            return false;

        ble2mqtt->devices[i]->appid = (uint8_t)i;
        bzero(&ble2mqtt->devices[i]->name, BLE2MQTT_DEV_MAX_NAME + 1);
        bzero(&ble2mqtt->devices[i]->address_str, BTL2MQTT_DEV_ADDR_LEN + 1);
        bzero(&ble2mqtt->devices[i]->address, ESP_BD_ADDR_LEN);
        ble2mqtt->devices[i]->address_type = BLE_ADDR_TYPE_RANDOM;
        ble2mqtt->devices[i]->gattc_if = ESP_GATT_IF_NONE;
        ble2mqtt->devices[i]->is_connected = false;
        ble2mqtt->devices[i]->is_connecting = false;
        ble2mqtt->devices[i]->conn_id = 0xFFFF;
        ble2mqtt->devices[i]->service_uuid.len = 0;
        ble2mqtt->devices[i]->service_start_handle = 0;
        ble2mqtt->devices[i]->service_end_handle = 0;
        ble2mqtt->devices[i]->char_elem_result = NULL;
        ble2mqtt->devices[i]->descr_elem_result = NULL;
    }

#ifdef ENABLEBTDEVTEST
    ble2mqtt->devices[0]->appid = 0;
    strcpy(ble2mqtt->devices[0]->name, "Nordic_Blinky");
    strcpy(ble2mqtt->devices[0]->address_str, "F1:34:D5:DF:B8:3B");
    bzero(&ble2mqtt->devices[0]->address, ESP_BD_ADDR_LEN);
    ble2mqtt->devices[0]->address_type = BLE_ADDR_TYPE_PUBLIC;
    ble2mqtt->devices[0]->is_connected = false;
    ble2mqtt->devices[0]->is_connecting = false;
    ble2mqtt->devices[0]->gattc_if = ESP_GATT_IF_NONE;
    ble2mqtt->devices[0]->conn_id = 0xFFFF;
    ble2mqtt->devices_len = 1;
    ble2mqtt->devices[0]->service_start_handle = 0;
    ble2mqtt->devices[0]->service_end_handle = 0;
    ble2mqtt->devices[0]->char_elem_result = NULL;
    ble2mqtt->devices[0]->descr_elem_result = NULL;

    ble2mqtt->devices[0]->service_uuid.len = 0;
    xEventGroupSetBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT);
#endif
    return true;
}
