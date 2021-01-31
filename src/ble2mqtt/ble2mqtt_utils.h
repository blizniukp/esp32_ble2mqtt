#ifndef HEADER_BLE2MQTT_UTILS
#define HEADER_BLE2MQTT_UTILS

#include <string.h>
#include "esp_gatt_defs.h"
#include "freertos/FreeRTOS.h"

/**
 * Convert string to esp_bt_uuid_t
 * @param value [in] - service uuid as string (eg. "beb5483e-36e1-4688-b7f5-ea07361b26a8")
 * @param m_uuid[out] - pointer where store parsing result
 * @return True if no error else False
 */
bool ble2mqtt_utils_parse_uuid(char *value, esp_bt_uuid_t *m_uuid);

#endif