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
        ESP_LOGI(TAG,"---------------------------------------------");
        bits = xEventGroupGetBits(ble2mqtt->s_event_group);
        ESP_LOGI(TAG, "Wifi: %c, Mqtt: %c", BITCHECK(bits, BLE2MQTT_WIFI_CONNECTED_BIT), BITCHECK(bits, BLE2MQTT_MQTT_CONNECTED_BIT));

        if (xSemaphoreTake(ble2mqtt->xMutexDevices, (TickType_t)10) == pdTRUE)
        {
            if (ble2mqtt->devices_len)
            {
                for (int i = 0; i < ble2mqtt->devices_len; i++)
                {
                    bledevice_t *dev = ble2mqtt->devices[i];
                    ESP_LOGI(TAG, "Dev[%i]. Connected: %s, Name: %s, Addr: %s, AddrType: %d, If: %u",
                             i, (dev->is_connected == true ? "Y" : "N"), dev->name, dev->address_str, dev->address_type, dev->gattc_if);
                    if (dev->service_uuid.len != 0)
                    {
                        switch (dev->service_uuid.len)
                        {
                        case ESP_UUID_LEN_16:
                            ESP_LOGI(TAG, "16B: %2X", dev->service_uuid.uuid.uuid16);
                            break;
                        case ESP_UUID_LEN_32:
                            ESP_LOGI(TAG, "32B: %4X", dev->service_uuid.uuid.uuid32);
                            break;
                        case ESP_UUID_LEN_128:
                        {
                            ESP_LOGI(TAG, "128B: ");
                            esp_log_buffer_hex(TAG, dev->service_uuid.uuid.uuid128, ESP_UUID_LEN_128);
                        }
                        break;
                        default:
                            ESP_LOGE(TAG, "Unknown service UUID");
                            break;
                        }
                    }
                }
            }
            xSemaphoreGive(ble2mqtt->xMutexDevices);
        }
        ESP_LOGI(TAG,"---------------------------------------------");
        vTaskDelay(15000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}