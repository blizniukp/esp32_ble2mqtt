#include <string.h>
#include "esp_gatt_defs.h"
#include "freertos/FreeRTOS.h"

#include "ble2mqtt_utils.h"

bool ble2mqtt_utils_parse_address(char *address_str, uint8_t *address)
{
    if (strlen(address_str) != 17)
        return false;

    int data[6];
    sscanf(address_str, "%x:%x:%x:%x:%x:%x", &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);
    address[0] = (uint8_t)data[0];
    address[1] = (uint8_t)data[1];
    address[2] = (uint8_t)data[2];
    address[3] = (uint8_t)data[3];
    address[4] = (uint8_t)data[4];
    address[5] = (uint8_t)data[5];

    return true;
}

bool ble2mqtt_utils_parse_uuid(char *value, esp_bt_uuid_t *m_uuid)
{
    if (!m_uuid)
        return NULL;

    int val_len = strlen(value);
    if (val_len == 4)
    {
        m_uuid->len = ESP_UUID_LEN_16;
        m_uuid->uuid.uuid16 = 0;
        for (int i = 0; i < val_len;)
        {
            uint8_t MSB = value[i];
            uint8_t LSB = value[i + 1];

            if (MSB > '9')
                MSB -= 7;
            if (LSB > '9')
                LSB -= 7;
            m_uuid->uuid.uuid16 += (((MSB & 0x0F) << 4) | (LSB & 0x0F)) << (2 - i) * 4;
            i += 2;
        }
        return true;
    }
    else if (val_len == 8)
    {
        m_uuid->len = ESP_UUID_LEN_32;
        m_uuid->uuid.uuid32 = 0;
        for (int i = 0; i < val_len;)
        {
            uint8_t MSB = value[i];
            uint8_t LSB = value[i + 1];

            if (MSB > '9')
                MSB -= 7;
            if (LSB > '9')
                LSB -= 7;
            m_uuid->uuid.uuid32 += (((MSB & 0x0F) << 4) | (LSB & 0x0F)) << (6 - i) * 4;
            i += 2;
        }
        return true;
    }
    else if (val_len == 16)
    {
        m_uuid->len = ESP_UUID_LEN_128;
        m_uuid->uuid.uuid32 = 0;
        for (int i = 0; i < val_len;)
        {
            uint8_t MSB = value[i];
            uint8_t LSB = value[i + 1];

            if (MSB > '9')
                MSB -= 7;
            if (LSB > '9')
                LSB -= 7;
            m_uuid->uuid.uuid32 += (((MSB & 0x0F) << 4) | (LSB & 0x0F)) << (6 - i) * 4;
            i += 2;
        }
        return true;
    }
    else if (val_len == 36)
    {
        m_uuid->len = ESP_UUID_LEN_128;
        int n = 0;
        for (int i = 0; i < val_len;)
        {
            if (value[i] == '-')
                i++;
            uint8_t MSB = value[i];
            uint8_t LSB = value[i + 1];

            if (MSB > '9')
                MSB -= 7;
            if (LSB > '9')
                LSB -= 7;
            m_uuid->uuid.uuid128[15 - n++] = ((MSB & 0x0F) << 4) | (LSB & 0x0F);
            i += 2;
        }
        return true;
    }
    return false;
}

