#ifdef UNIT_TEST

//#include "ble2mqtt/ble2mqtt_utils.h"
#include "../src/ble2mqtt/ble2mqtt_utils.h"
#include <unity.h>

void test_function_parse_uuid_len2(void)
{
    esp_bt_uuid_t *result = ble2mqtt_utils_parse_uuid("00FF");

    TEST_ASSERT_NOT_NULL(result);

}

int app_main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_function_parse_uuid_len2);
    UNITY_END();

    return 0;
}
#endif