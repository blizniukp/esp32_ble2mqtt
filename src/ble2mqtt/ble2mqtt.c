#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "ble2mqtt.h"

static const char *TAG = "ble2mqtt";

ble2mqtt_t *ble2mqtt_create(void)
{
    ble2mqtt_t *ble2mqtt = malloc(sizeof(ble2mqtt_t));
    return ble2mqtt;
}

bool ble2mqtt_init(ble2mqtt_t *ble2mqtt)
{
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
        ble2mqtt->devices[i]->name[0] = 0;
        ble2mqtt->devices[i]->mac[0] = 0;
        ble2mqtt->devices[i]->name[BLE2MQTT_DEV_MAX_NAME] = 0;
        ble2mqtt->devices[i]->mac[BTL2MQTT_DEV_MAC_LEN] = 0;
    }
    return true;
}