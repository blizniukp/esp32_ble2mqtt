#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "tled.h"

static const char *TAG = "tled";

#define LED_BUILTIN (GPIO_NUM_2)
#define BITCHECK(byte, nbit) (((byte) & (nbit)) == (nbit) ? 'Y' : 'N')

#define LEDC_HS_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_HS_TIMER LEDC_TIMER_0
#define LEDC_LS_MODE LEDC_LOW_SPEED_MODE
#define LEDC_LS_TIMER LEDC_TIMER_1
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH0_GPIO (LED_BUILTIN)
#define LEDC_TEST_CH_NUM (1)
#define LEDC_TEST_FADE_TIME (2000)
#define LEDC_TEST_DUTY (4000)

ledc_channel_config_t ledc;
bool led_configured = false;

void tled_on()
{
    if (!led_configured)
        return;
    ledc_set_duty(ledc.speed_mode, ledc.channel, LEDC_TEST_DUTY);
    ledc_update_duty(ledc.speed_mode, ledc.channel);
}

void tled_off()
{
    if (!led_configured)
        return;
    ledc_set_duty(ledc.speed_mode, ledc.channel, 0);
    ledc_update_duty(ledc.speed_mode, ledc.channel);
}

void vTaskLed(void *pvParameters)
{
    ESP_LOGI(TAG, "Run task led");

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_HS_MODE,
        .timer_num = LEDC_HS_TIMER
    };
    ledc_timer_config(&ledc_timer);

    ledc_timer.speed_mode = LEDC_LS_MODE;
    ledc_timer.timer_num = LEDC_LS_TIMER;
    ledc_timer_config(&ledc_timer);

    ledc.channel = LEDC_HS_CH0_CHANNEL;
    ledc.duty = 0;
    ledc.gpio_num = LEDC_HS_CH0_GPIO;
    ledc.speed_mode = LEDC_HS_MODE;
    ledc.timer_sel = LEDC_HS_TIMER;

    ledc_channel_config(&ledc);
    ledc_fade_func_install(0);

    led_configured = true;
    while (1)
    {
        ledc_set_fade_with_time(ledc.speed_mode,
                                ledc.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
        ledc_fade_start(ledc.speed_mode,
                        ledc.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        ledc_set_fade_with_time(ledc.speed_mode,
                                ledc.channel, 0, LEDC_TEST_FADE_TIME);
        ledc_fade_start(ledc.speed_mode,
                        ledc.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        vTaskDelay((1000 * 30) / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}
