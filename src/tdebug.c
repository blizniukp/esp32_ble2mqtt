#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "tdebug.h"
#include "ble2mqtt/ble2mqtt.h"

static const char *TAG = "tdebug";

#define BITCHECK(byte, nbit) (((byte) & (nbit)) == (nbit) ? 'Y' : 'N')

void vTaskDebug(void *pvParameters)
{
    ble2mqtt_t *ble2mqtt = (ble2mqtt_t *)pvParameters;
    ESP_LOGI(TAG, "Run task debug");
    EventBits_t bits;

    while (1)
    {
        bits = xEventGroupGetBits(ble2mqtt->s_event_group);
        ESP_LOGI(TAG, "Wifi: %c, Mqtt: %c", BITCHECK(bits, BLE2MQTT_WIFI_CONNECTED_BIT), BITCHECK(bits, BLE2MQTT_MQTT_CONNECTED_BIT));

        if (xSemaphoreTake(ble2mqtt->xMutexDevices, (TickType_t)10) == pdTRUE)
        {
            if (ble2mqtt->devices_len)
            {
                for (int i = 0; i < ble2mqtt->devices_len; i++)
                {
                    ESP_LOGI(TAG, "Dev[%i]. Connected: %s, Name: %s, Addr: %s, AddrType: %d, If: %u",
                             i, (ble2mqtt->devices[i]->is_connected == true ? "Y" : "N"), ble2mqtt->devices[i]->name, ble2mqtt->devices[i]->address_str, ble2mqtt->devices[i]->address_type, ble2mqtt->devices[i]->gattc_if);
                }
            }
            xSemaphoreGive(ble2mqtt->xMutexDevices);
        }
        vTaskDelay(15000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}