#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "twifi.h"

static const char *TAG = "twifi";

void vTaskWifi(void *pvParameters)
{
    ESP_LOGI(TAG, "Run task wifi");
    if (ESP_OK != esp_wifi_set_mode(WIFI_MODE_STA))
    {
    }
    while (1)
    {
        //TODO: dodać obsługę WiFi
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}