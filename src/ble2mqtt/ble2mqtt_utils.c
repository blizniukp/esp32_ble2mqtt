#include <string.h>
#include "esp_gatt_defs.h"
#include "freertos/FreeRTOS.h"

#include "ble2mqtt_utils.h"

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