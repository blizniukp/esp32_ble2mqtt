#ifndef HEADER_BLE2MQTT
#define HEADER_BLE2MQTT

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_gattc_api.h"

#define BLE2MQTT_WIFI_CONNECTED_BIT BIT0       /* Connected to WiFi */
#define BLE2MQTT_MQTT_CONNECTED_BIT BIT1       /* Connected to MQTT Broker */
#define BLE2MQTT_MQTT_GOT_BLEDEV_LIST_BIT BIT2 /* Got device list from HomeAssistant */

#define BLE2MQTT_DEV_MAX_NAME (20)      /* Max btle device name */
#define BTL2MQTT_DEV_ADDR_LEN (17)      /* Address length */
#define BTL2MQTT_DEV_ADDR_TYPE_LEN (10) /* Address type length */
#define BTL2MQTT_DEV_MAX_LEN (4)        /* Max supported devices */

/**
 * Information about BTLE device
 */
typedef struct bledevice_def
{
    int appid;

    char name[BLE2MQTT_DEV_MAX_NAME + 1];        /* Device name */
    char address_str[BTL2MQTT_DEV_ADDR_LEN + 1]; /* Device address (as string) */
    esp_bd_addr_t address;                       /* Device address */
    esp_ble_addr_type_t address_type;            /* Address type */

    esp_bt_uuid_t service_uuid; /* Serivce UUID */
    esp_bt_uuid_t char_uuid;    /* Characteristic UUID */

    bool is_connecting; /* If True, esp trying to connect to device */
    bool is_connected;  /* If True, we are connected to bt device */

    uint16_t gattc_if; /* Gatt interface */
} bledevice_t;

typedef struct ble2mqtt_def
{
    EventGroupHandle_t s_event_group;

    SemaphoreHandle_t xMutexDevices; /* Mutex on 'devices' */
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