#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "tmqtt.h"
#include "ble2mqtt/ble2mqtt.h"
#include "ble2mqtt_config.h"

static const char *TAG = "tmqtt";

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    ble2mqtt_t *ble2mqtt = (ble2mqtt_t *)event->user_context;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_SUBSCRIBE, MQTT_QOS);
        ESP_LOGD(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        xEventGroupSetBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupClearBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_CONNECTED_BIT);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void vTaskMqtt(void *pvParameters)
{
    ble2mqtt_t *ble2mqtt = (ble2mqtt_t *)pvParameters;
    ESP_LOGI(TAG, "Run task mqtt");
    EventBits_t bits;
    int msg_id;

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROKER_URI,
        .user_context = (void *)ble2mqtt,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    while (1)
    {
        bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY); //Wait for wifi
        if (!(bits & BLE2MQTT_WIFI_CONNECTED_BIT))
            continue;

        ESP_LOGI(TAG, "Try to connect to MQTT");
        esp_mqtt_client_start(client);
        bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, (5000 / portTICK_PERIOD_MS)); //Wait 5 sec for mqtt
        if (!(bits & BLE2MQTT_MQTT_CONNECTED_BIT))
            continue;

        ESP_LOGI(TAG, "Try to get dev list");
        msg_id = esp_mqtt_client_publish(client, "/ble2mqtt/app/getDevList", "{}", 0, MQTT_QOS, 1);
        ESP_LOGD(TAG, "Pub message, msg_id=%d", msg_id);
        bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_GOT_BLEDEV_LIST_BIT, pdFALSE, pdFALSE, (15000 / portTICK_PERIOD_MS)); //Wait 15 sec for btle device list
        if (!(bits & BLE2MQTT_GOT_BLEDEV_LIST_BIT))
            continue;

        ESP_LOGI(TAG, "Got device list");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}