#ifndef HEADER_BLE2MQTT_UTILS
#define HEADER_BLE2MQTT_UTILS

#include <string.h>
#include "esp_gatt_defs.h"
#include "freertos/FreeRTOS.h"

/**
 * Convert BT address from string to esp_bd_addr_t
 * @param address_str [in] - device address as char array
 * @param address [out] - pointer to array to store result
 * @return True on success
 */
bool ble2mqtt_utils_parse_address(char *address_str, uint8_t *address);

/**
 * Convert string to esp_bt_uuid_t
 * @param value [in] - service uuid as string (eg. "beb5483e-36e1-4688-b7f5-ea07361b26a8")
 * @param m_uuid[out] - pointer where store parsing result
 * @return True if no error else False
 */
bool ble2mqtt_utils_parse_uuid(char *value, esp_bt_uuid_t *m_uuid);

/**
 * Convert uint8_t array to hex string
 * @param dest [out]
 * @param dest_len [out]
 * @param values [in]
 * @param val_len [in]
 * @return True if no error else False
 */
bool ble2mqtt_utils_u8_to_hex(char* dest, size_t dest_len, const uint8_t* values, size_t val_len);
#endif