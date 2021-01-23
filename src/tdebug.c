#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"

#include "tdebug.h"
#include "ble2mqtt/ble2mqtt.h"

static const char *TAG = "tdebug";

void vTaskDebug(void *pvParameters)
{
    ble2mqtt_t *ble2mqtt = (ble2mqtt_t *)pvParameters;
    ESP_LOGI(TAG, "Run task debug");
    EventBits_t bits;

    while (1)
    {
        bits = xEventGroupGetBits(ble2mqtt->s_event_group);
        ESP_LOGI(TAG, "\tConnected Bit: %d, Disconnected Bit: %d", (bits & BLE2MQTT_WIFI_CONNECTED_BIT), (bits & BLE2MQTT_WIFI_DISCONNECTED_BIT));
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}