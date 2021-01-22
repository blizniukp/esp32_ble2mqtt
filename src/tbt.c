#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"

#include "tbt.h"

static const char *TAG = "tbt";

void vTaskBt(void *pvParameters)
{
    ESP_LOGI(TAG, "Run task bluetooth");
    while (1)
    {
        //TODO: dodać obsługę BT
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}