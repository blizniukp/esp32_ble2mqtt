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
        bzero(&ble2mqtt->devices[i]->name, BLE2MQTT_DEV_MAX_NAME + 1);
        bzero(&ble2mqtt->devices[i]->address, BTL2MQTT_DEV_ADDR_LEN + 1);
        ble2mqtt->devices[i]->address_type = BLE_ADDR_TYPE_RANDOM;
        ble2mqtt->bt.gattc_if = ESP_GATT_IF_NONE;
    }

#ifdef ENABLEBTDEVTEST
    strcpy(ble2mqtt->devices[0]->name, "Nordic_Blinky");
    strcpy(ble2mqtt->devices[0]->address, "F1:34:D5:DF:B8:3B");
    ble2mqtt->devices[0]->address_type = BLE_ADDR_TYPE_PUBLIC;
    ble2mqtt->bt.gattc_if = ESP_GATT_IF_NONE;
    ble2mqtt->devices_len = 1;
#endif
    return true;
}