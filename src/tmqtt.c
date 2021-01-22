#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"

#include "tmqtt.h"

static const char *TAG = "tmqtt";

void vTaskMqtt(void *pvParameters)
{
    ESP_LOGI(TAG, "Run task mqtt");
    while (1)
    {
        //TODO: dodać obsługę MQTT
        vTaskDelay(1700 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}