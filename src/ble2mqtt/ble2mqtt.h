#ifndef HEADER_BLE2MQTT
#define HEADER_BLE2MQTT

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_DISCONNECTED_BIT BIT1
#define WIFI_FAIL_BIT BIT2

typedef struct ble2mqtt_def
{
    EventGroupHandle_t s_event_group;
} ble2mqtt_t;

static ble2mqtt_t ble2mqtt;

/**
 * Initialize ble2mqtt struct
 */
void ble2mqtt_init(void);

#endif