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
#include "driver/gpio.h"

#include "twifi.h"
#include "tbt.h"
#include "tmqtt.h"
#include "tb2mconfig.h"
#include "ble2mqtt/ble2mqtt.h"
#ifndef DISABLETDEBUG
#include "tdebug.h"
#endif
#include "tled.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Run app");
#ifdef PRINTCHIPINFO
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

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT));
    int reset_config = gpio_get_level(GPIO_NUM_4);

    b2mconfig_t *b2mconfig = b2mconfig_create();
    if (b2mconfig == NULL)
    {
        ESP_LOGE(TAG, "malloc error!");
        while (1)
            ;
    }
    else
    {
        b2mconfig_load(b2mconfig);
    }

    if (reset_config == 1 || b2mconfig->not_initialized)
    {
        xTaskCreate(vTaskLed, "task_led", 2048, NULL, 20, NULL);
        tled_on();
        xTaskCreate(vTaskB2MConfig, "task_b2mconfig", (2048 * 6), (void *)b2mconfig, 8, NULL);
    }
    else
    {
        ble2mqtt_t *ble2mqtt = ble2mqtt_create();
        if (!ble2mqtt)
        {
            ESP_LOGE(TAG, "malloc error!");
            while (1)
                ;
        }
        if (ble2mqtt_init(ble2mqtt, b2mconfig) == false)
        {
            ESP_LOGE(TAG, "ble2mqtt_init error!");
            while (1)
                ;
        }

#ifndef DISABLETWIFI
        xTaskCreate(vTaskWifi, "task_wifi", (2048 * 6), (void *)ble2mqtt, 8, NULL);
#endif
#ifndef DISABLETMQTT
        xTaskCreate(vTaskMqtt, "task_mqtt", (2048 * 6), (void *)ble2mqtt, 10, NULL);
#endif
#ifndef DISABLETBT
        xTaskCreate(vTaskBt, "task_bt", (2048 * 6), (void *)ble2mqtt, 12, NULL);
#endif
#ifndef DISABLETDEBUG
        xTaskCreate(vTaskDebug, "task_debug", 2048, (void *)ble2mqtt, 20, NULL);
#endif
        xTaskCreate(vTaskLed, "task_led", 2048, (void *)ble2mqtt, 20, NULL);
    }
}
