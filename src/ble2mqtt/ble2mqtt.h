#ifndef HEADER_BLE2MQTT
#define HEADER_BLE2MQTT

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define BLE2MQTT_WIFI_CONNECTED_BIT BIT0  /* Connected to WiFi */
#define BLE2MQTT_MQTT_CONNECTED_BIT BIT1  /* Connected to MQTT Broker */
#define BLE2MQTT_GOT_BLEDEV_LIST_BIT BIT2 /* Got device list from HomeAssistant */

#define BLE2MQTT_DEV_MAX_NAME (10) /* Max btle device name */
#define BTL2MQTT_DEV_MAC_LEN (17)  /* Mac length */
#define BTL2MQTT_DEV_MAX_LEN (6)   /* Max supported devices */

/**
 * Information about BTLE device
 */
typedef struct bledevice_def
{
    char name[BLE2MQTT_DEV_MAX_NAME + 1]; /* Device name */
    char mac[BTL2MQTT_DEV_MAC_LEN + 1];   /* Mac address */
} bledevice_t;

typedef struct ble2mqtt_def
{
    EventGroupHandle_t s_event_group;

    SemaphoreHandle_t xMutexDevices; /* Mutex on devices */
    bledevice_t *devices[BTL2MQTT_DEV_MAX_LEN];
    uint8_t devices_len; /* Number of items in 'devices' array */
} ble2mqtt_t;

/**
 * Create instance of ble2mqtt_t struct
 */
ble2mqtt_t *ble2mqtt_create(void);

/**
 * Initialize ble2mqtt struct
 * @return - true if init success
 */
bool ble2mqtt_init(ble2mqtt_t *ble2mqtt);

#endif