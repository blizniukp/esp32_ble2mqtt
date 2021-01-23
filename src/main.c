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
#include "tbt.h"
#include "tmqtt.h"
#include "ble2mqtt/ble2mqtt.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Run app");
#ifdef DEBUG
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    printf("silicon revision %d, ", chip_info.revision);
    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
    fflush(stdout);
#endif

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ble2mqtt_t *ble2mqtt = ble2mqtt_create();
    if (!ble2mqtt)
    {
        ESP_LOGE(TAG, "malloc error!");
        while (1)
            ;
    }

    xTaskCreate(vTaskWifi, "task_wifi", (2048 * 6), (void *)ble2mqtt, 8, NULL);
    xTaskCreate(vTaskMqtt, "task_mqtt", 2048, (void *)ble2mqtt, 10, NULL);
    xTaskCreate(vTaskBt, "task_bt", 2048, (void *)ble2mqtt, 12, NULL);
}
