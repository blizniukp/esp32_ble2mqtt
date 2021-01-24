#ifndef HEADER_BLE2MQTT
#define HEADER_BLE2MQTT

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define BLE2MQTT_WIFI_CONNECTED_BIT BIT0    /*!< Connected to WiFi */
#define BLE2MQTT_MQTT_CONNECTED_BIT BIT1    /*!< Connected to MQTT Broker */
#define BLE2MQTT_GOT_BLEDEV_LIST_BIT BIT2   /*!< Got device list from HomeAssistant */

typedef struct ble2mqtt_def
{
    EventGroupHandle_t s_event_group;
} ble2mqtt_t;

/**
 * Create instance of ble2mqtt_t struct
 */
ble2mqtt_t* ble2mqtt_create(void);

/**
 * Initialize ble2mqtt struct
 */
void ble2mqtt_init(ble2mqtt_t *ble2mqtt);

#endif