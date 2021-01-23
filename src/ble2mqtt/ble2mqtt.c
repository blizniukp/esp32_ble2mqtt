#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "ble2mqtt.h"

static const char *TAG = "ble2mqtt";

ble2mqtt_t *ble2mqtt_create(void)
{
    ble2mqtt_t *ble2mqtt = malloc(sizeof(ble2mqtt_t));
    return ble2mqtt;
}

void ble2mqtt_init(ble2mqtt_t *ble2mqtt)
{
    ble2mqtt->s_event_group = xEventGroupCreate();
}