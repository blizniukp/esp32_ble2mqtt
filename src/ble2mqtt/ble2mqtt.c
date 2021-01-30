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

        ble2mqtt->devices[i]->appid = i;
        bzero(&ble2mqtt->devices[i]->name, BLE2MQTT_DEV_MAX_NAME + 1);
        bzero(&ble2mqtt->devices[i]->address_str, BTL2MQTT_DEV_ADDR_LEN + 1);
        bzero(&ble2mqtt->devices[i]->address, ESP_BD_ADDR_LEN);
        ble2mqtt->devices[i]->address_type = BLE_ADDR_TYPE_RANDOM;
        ble2mqtt->devices[i]->gattc_if = ESP_GATT_IF_NONE;
        ble2mqtt->devices[i]->is_connected = false;
        ble2mqtt->devices[i]->is_connecting = false;
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
    ble2mqtt->devices_len = 1;
    xEventGroupSetBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT);
#endif
    return true;
}

bool bt_parse_address(bledevice_t *device)
{
    if (strlen(device->address_str) != 17)
        return false;

    int data[6];
    sscanf(device->address_str, "%x:%x:%x:%x:%x:%x", &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);
    device->address[0] = (uint8_t)data[0];
    device->address[1] = (uint8_t)data[1];
    device->address[2] = (uint8_t)data[2];
    device->address[3] = (uint8_t)data[3];
    device->address[4] = (uint8_t)data[4];
    device->address[5] = (uint8_t)data[5];

    return true;
}