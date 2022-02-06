#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <cJSON.h>

#include "tmqtt.h"
#include "ble2mqtt/ble2mqtt.h"
#include "ble2mqtt/ble2mqtt_utils.h"
#include "ble2mqtt_config.h"
#include "led_indicator.h"

static const char *TAG = "tmqtt";
#define VAL_MAX (40)
#define MSG_MAX (200)

/**
 * Parse new message and add to queue
 */
static void mqtt_new_message(ble2mqtt_t *ble2mqtt, esp_mqtt_event_handle_t event)
{
    ESP_LOGD(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
    ESP_LOGD(TAG, "DATA=%.*s", event->data_len, event->data);
    if (event->data_len == 0 || event->topic_len == 0)
        return;

    if (strstr(event->topic, "devList")) //Got device list
    {
        ESP_LOGD(TAG, "Got devList");
        if (xSemaphoreTake(ble2mqtt->xMutexDevices, (TickType_t)10) == pdTRUE)
        {
            ble2mqtt->devices_len = 0;
            cJSON *root = cJSON_Parse(event->data);
            int dev_arr_len = cJSON_GetArraySize(root);
            ESP_LOGD(TAG, "dev_arr_len: %d\n", dev_arr_len);
            int p = 0;
            for (int i = 0; i < dev_arr_len; i++, p++)
            {
                cJSON *dev = cJSON_GetArrayItem(root, i);
                strncpy(ble2mqtt->devices[p]->name, cJSON_GetObjectItem(dev, "name")->valuestring, BLE2MQTT_DEV_MAX_NAME);
                strncpy(ble2mqtt->devices[p]->address_str, cJSON_GetObjectItem(dev, "address")->valuestring, BTL2MQTT_DEV_ADDR_LEN);
                if (strncasecmp(cJSON_GetObjectItem(dev, "address_type")->valuestring, "public", BTL2MQTT_DEV_ADDR_TYPE_LEN) == 0)
                    ble2mqtt->devices[p]->address_type = BLE_ADDR_TYPE_PUBLIC;
                else if (strncasecmp(cJSON_GetObjectItem(dev, "address_type")->valuestring, "random", BTL2MQTT_DEV_ADDR_TYPE_LEN) == 0)
                    ble2mqtt->devices[p]->address_type = BLE_ADDR_TYPE_RANDOM;
                else if (strncasecmp(cJSON_GetObjectItem(dev, "address_type")->valuestring, "rpa-public", BTL2MQTT_DEV_ADDR_TYPE_LEN) == 0)
                    ble2mqtt->devices[p]->address_type = BLE_ADDR_TYPE_RPA_PUBLIC;
                else if (strncasecmp(cJSON_GetObjectItem(dev, "address_type")->valuestring, "rpa-random", BTL2MQTT_DEV_ADDR_TYPE_LEN) == 0)
                    ble2mqtt->devices[p]->address_type = BLE_ADDR_TYPE_RPA_RANDOM;
                else
                    ble2mqtt->devices[p]->address_type = BLE_ADDR_TYPE_RANDOM;
                if (!ble2mqtt_utils_parse_address(ble2mqtt->devices[p]->address_str, &ble2mqtt->devices[p]->address))
                {
                    ESP_LOGE(TAG, "Incorrect bt address %s", ble2mqtt->devices[p]->address_str);
                    p--;
                    continue;
                }
                if (!ble2mqtt_utils_parse_uuid(cJSON_GetObjectItem(dev, "service_uuid")->valuestring, &ble2mqtt->devices[p]->service_uuid))
                {
                    ESP_LOGE(TAG, "Service uuid is incorrect %s", cJSON_GetObjectItem(dev, "service_uuid")->valuestring);
                    p--;
                    continue;
                }
                if (!ble2mqtt_utils_parse_uuid(cJSON_GetObjectItem(dev, "char_uuid")->valuestring, &ble2mqtt->devices[p]->char_uuid))
                {
                    ESP_LOGE(TAG, "Characteristic uuid is incorrect %s", cJSON_GetObjectItem(dev, "char_uuid")->valuestring);
                    p--;
                    continue;
                }
                ble2mqtt->devices_len++;
                if (ble2mqtt->devices_len >= BTL2MQTT_DEV_MAX_LEN)
                {
                    ESP_LOGE(TAG, "Max devices!");
                    break;
                }
            }
            cJSON_Delete(root);
            xSemaphoreGive(ble2mqtt->xMutexDevices);
            xEventGroupSetBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT);
        }
    }
}

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
        mqtt_new_message(ble2mqtt, event);
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
        .user_context = (void *)ble2mqtt,
        .transport = MQTT_TRANSPORT_OVER_TCP,
        .protocol_ver = MQTT_PROTOCOL_V_3_1_1};
    mqtt_cfg.host = ble2mqtt->b2mconfig->broker_ip_address;
    mqtt_cfg.username = ble2mqtt->b2mconfig->broker_username;
    mqtt_cfg.password = ble2mqtt->b2mconfig->broker_password;
    mqtt_cfg.port = ble2mqtt->b2mconfig->broker_port;

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    while (1)
    {
        if (!(xEventGroupGetBits(ble2mqtt->s_event_group) & BLE2MQTT_WIFI_CONNECTED_BIT))
        {
            bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY); //Wait for wifi
            if (!(bits & BLE2MQTT_WIFI_CONNECTED_BIT))
                continue;
        }
        if (!(xEventGroupGetBits(ble2mqtt->s_event_group) & BLE2MQTT_MQTT_CONNECTED_BIT))
        {
            ESP_LOGI(TAG, "Try to connect to MQTT");
            esp_mqtt_client_start(client);
            bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, (5000 / portTICK_PERIOD_MS)); //Wait 5 sec for mqtt
            if (!(bits & BLE2MQTT_MQTT_CONNECTED_BIT))
                continue;
        }
        if (!(xEventGroupGetBits(ble2mqtt->s_event_group) & BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT))
        {
            ESP_LOGI(TAG, "Try to get dev list");
            msg_id = esp_mqtt_client_publish(client, "/ble2mqtt/app/getDevList", "{}", 0, MQTT_QOS, 1);
            ESP_LOGD(TAG, "Pub message, msg_id=%d", msg_id);
            bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT, pdFALSE, pdFALSE, (15000 / portTICK_PERIOD_MS)); //Wait 15 sec for btle device list
            if (!(bits & BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT))
                continue;
            ESP_LOGI(TAG, "Got device list");
        }

        ESP_LOGI(TAG, "Wait for new data in queue");
        bits = xEventGroupWaitBits(ble2mqtt->s_event_group, BLE2MQTT_NEW_BTLE_MESSAGE, pdTRUE, pdFALSE, (5000 / portTICK_PERIOD_MS)); //Wait 5 sec for bt data
        if (!(bits & BLE2MQTT_NEW_BTLE_MESSAGE))
            continue;

        ESP_LOGD(TAG, "New data in queue");
        btle_q_element_t *elem = NULL;
        if (xQueueReceive(ble2mqtt->xQueue,
                          &elem,
                          (TickType_t)0) == pdPASS)
        {
            ESP_LOGD(TAG, "elem: is_notify: %d, value[0] : %u", elem->is_notify, elem->value[0]);

            char msg[MSG_MAX] = {0};
            char val[VAL_MAX] = {0};
            if (!ble2mqtt_utils_u8_to_hex(val, VAL_MAX, elem->value, elem->value_len))
            {
                ESP_LOGE(TAG, "Convert error");
                free(elem->value);
                free(elem);
                continue;
            }

            sprintf(msg, "{\"address\": \"%2X:%2X:%2X:%2X:%2X:%2X\", \"is_notify\": %s, \"val_len\": %hu, \"val\": \"%s\"}",
                    elem->address[0], elem->address[1],
                    elem->address[2], elem->address[3],
                    elem->address[4], elem->address[5],
                    (elem->is_notify == true ? "true" : "false"),
                    elem->value_len,
                    val);

            free(elem->value);
            free(elem);
            esp_mqtt_client_publish(client, "/ble2mqtt/app/value", msg, strlen(msg), MQTT_QOS, 1);
            led_indicator_off();
        }
        else
        {
            ESP_LOGE(TAG, "xQueueReceive error");
        }

        continue;

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}