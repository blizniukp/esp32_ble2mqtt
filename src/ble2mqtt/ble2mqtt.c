#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "ble2mqtt.h"

void ble2mqtt_init(void){
    ble2mqtt.s_event_group = xEventGroupCreate();
}