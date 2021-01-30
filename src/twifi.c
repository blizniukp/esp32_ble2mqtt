#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "twifi.h"
#include "ble2mqtt_config.h"
#include "ble2mqtt/ble2mqtt.h"

static const char *TAG = "twifi";

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    ble2mqtt_t *ble2mqtt = (ble2mqtt_t *)arg;

    if (event_base == WIFI_EVENT)
    {
        ESP_LOGI(TAG, "Got WIFI_EVENT. event_id: %d\n", event_id);
        if (WIFI_EVENT_STA_START == event_id)
        {
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
        else if (WIFI_EVENT_STA_DISCONNECTED == event_id)
        {
            xEventGroupClearBits(ble2mqtt->s_event_group, BLE2MQTT_WIFI_CONNECTED_BIT);
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
        else if (WIFI_EVENT_STA_CONNECTED == event_id)
        {
            ESP_LOGI(TAG, "connected to wifi");
        }
    }
    else if (event_base == IP_EVENT)
    {
        ESP_LOGI(TAG, "Got IP_EVENT. event_id: %d\n", event_id);
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(ble2mqtt->s_event_group, BLE2MQTT_WIFI_CONNECTED_BIT);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Unknown event");
    }
}

void vTaskWifi(void *pvParameters)
{
    ble2mqtt_t *ble2mqtt = (ble2mqtt_t *)pvParameters;

    ESP_LOGI(TAG, "Run task wifi");

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &event_handler,
                                               (void *)ble2mqtt));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &event_handler,
                                               (void *)ble2mqtt));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    while (1)
    {
#ifndef DISABLETASKMSG
        ESP_LOGD(TAG, "Task wifi");
#endif
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}