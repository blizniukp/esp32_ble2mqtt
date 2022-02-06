#include "driver/gpio.h"

#include "led_indicator.h"

#define LED_BUILTIN (GPIO_NUM_2)

void led_indicator_init()
{
    ESP_ERROR_CHECK(gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT));
}

void led_indicator_on()
{
    gpio_set_level(LED_BUILTIN, 0xFF);
}

void led_indicator_off()
{
    gpio_set_level(LED_BUILTIN, 0x00);
}