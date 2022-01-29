#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "http_server.h"
#include "mdns.h"

#include "b2mconfig.h"
#include "ble2mqtt_config.h"

static const char *TAG = "tb2mconfig";

#define TB2MCONFIG_WIFI_CLIENT_CONNECTED_BIT BIT0 /* Client connected to WiFi */

b2mconfig_t *b2mconfig_create(void)
{
    b2mconfig_t *b2mconfig = malloc(sizeof(b2mconfig_t));

    if (!b2mconfig)
        return NULL;

    b2mconfig->not_initialized = true;

    return b2mconfig;
}

static bool b2mconfig_load_parameter(nvs_handle *handle, char *pname, char **dest)
{
    size_t required_size = 0;
    esp_err_t err;

    err = nvs_get_str(*handle, pname, NULL, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Can't read parameter size %s from nvm", pname);
        *dest = NULL;
        return false;
    }
    ESP_LOGI(TAG, "nvm variable: %s, size: %zu", pname, required_size);
    *dest = malloc((sizeof(char) * required_size));
    if (*dest == NULL)
    {
        ESP_LOGE(TAG, "out of memory");
        return false;
    }

    err = nvs_get_str(*handle, pname, *dest, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Can't read parameter %s from nvm", pname);
        *dest = NULL;
        return false;
    }
    return true;
}

void b2mconfig_load(b2mconfig_t *b2mconfig)
{
    nvs_handle handle;
    esp_err_t err;
    char *init_status;
    b2mconfig->not_initialized = true;
    b2mconfig->wifi_ssid = NULL;
    b2mconfig->wifi_password = NULL;
    b2mconfig->broker_ip_address = NULL;

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "nvs_open done");

    if (b2mconfig_load_parameter(&handle, "ws", &b2mconfig->wifi_ssid) == false)
        return;
    if (b2mconfig_load_parameter(&handle, "wp", &b2mconfig->wifi_password) == false)
        return;
    if (b2mconfig_load_parameter(&handle, "bi", &b2mconfig->broker_ip_address) == false)
        return;
    if (b2mconfig_load_parameter(&handle, "bw", &b2mconfig->broker_password) == false)
        return;
    if (b2mconfig_load_parameter(&handle, "bp", &b2mconfig->broker_port) == false)
        return;
    if (b2mconfig_load_parameter(&handle, "bu", &b2mconfig->broker_username) == false)
        return;

    if (b2mconfig_load_parameter(&handle, "is", &init_status) == false)
        return;
    if (strncmp(init_status, "Y", 1) == 0)
        b2mconfig->not_initialized = false;
    ESP_LOGI(TAG, "The configuration has been loaded");
    ESP_LOGI(TAG, "wifi_ssid: \'%s\'", b2mconfig->wifi_ssid);
    ESP_LOGI(TAG, "wifi_password: \'%s\'", b2mconfig->wifi_password);
    ESP_LOGI(TAG, "broker_ip_address: \'%s\'", b2mconfig->broker_ip_address);
    ESP_LOGI(TAG, "broker_port: \'%s\'", b2mconfig->broker_port);
    ESP_LOGI(TAG, "broker_username: \'%s\'", b2mconfig->broker_username);
    ESP_LOGI(TAG, "broker_password: \'%s\'", b2mconfig->broker_password);
}

void b2mconfig_save(b2mconfig_t *b2mconfig)
{
    nvs_handle handle;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "nvs_open done");

    ESP_ERROR_CHECK(nvs_set_str(handle, "ws", b2mconfig->wifi_ssid));
    ESP_ERROR_CHECK(nvs_set_str(handle, "wp", b2mconfig->wifi_password));
    ESP_ERROR_CHECK(nvs_set_str(handle, "bi", b2mconfig->broker_ip_address));
    ESP_ERROR_CHECK(nvs_set_str(handle, "bw", b2mconfig->broker_password));
    ESP_ERROR_CHECK(nvs_set_str(handle, "bp", b2mconfig->broker_port));
    ESP_ERROR_CHECK(nvs_set_str(handle, "bu", b2mconfig->broker_username));
    ESP_ERROR_CHECK(nvs_set_str(handle, "is", "Y"));
    ESP_LOGI(TAG, "Commit changes");
    ESP_ERROR_CHECK(nvs_commit(handle));
}

static void event_ap_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    EventGroupHandle_t *s_event_group = (EventGroupHandle_t *)arg;
    if (event_base == WIFI_EVENT)
    {
        ESP_LOGI(TAG, "Got WIFI_EVENT. event_id: %d\n", event_id);
        switch (event_id)
        {
        case WIFI_EVENT_AP_STACONNECTED:
        {
            ESP_LOGI(TAG, "AP_STACONNECTED");
            xEventGroupSetBits(*s_event_group, TB2MCONFIG_WIFI_CLIENT_CONNECTED_BIT);
        }
        break;
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            ESP_LOGI(TAG, "AP_STADISCONNECTED");
            xEventGroupClearBits(*s_event_group, TB2MCONFIG_WIFI_CLIENT_CONNECTED_BIT);
        }
        break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        ESP_LOGI(TAG, "Got IP_EVENT. event_id: %d\n", event_id);
    }
    else
    {
        ESP_LOGE(TAG, "Unknown event");
    }
}

esp_err_t get_handler(httpd_req_t *req)
{
    const char *page = "<html>\
<head>\
<style>\
fieldset {\
  background-color: #eeeeee;\
}\
legend {\
  background-color: gray;\
  color: white;\
  padding: 5px 10px;\
}\
input {\
  margin: 5px;\
}\
</style>\
</head>\
<body>\
<h1>ble2mqtt</h1>\
<form action=\"/save\" method=\"post\">\
 <fieldset>\
  <legend>WiFi:</legend>\
  <label for=\"ws\">Ssid:</label>\
  <input type=\"text\" id=\"ws\" name=\"ws\"><br><br>\
  <label for=\"wp\">Password:</label>\
  <input type=\"password\" id=\"wp\" name=\"wp\">\
 </fieldset>\
 <fieldset>\
  <legend>Mqtt:</legend>\
  <label for=\"bi\">IP address:</label>\
  <input type=\"text\" id=\"bi\" name=\"bi\"><br><br>\
  <label for=\"bp\">Port:</label>\
  <input type=\"text\" id=\"bp\" name=\"bp\" value=\"1883\"><br><br>\
  <label for=\"bu\">User:</label>\
  <input type=\"text\" id=\"bu\" name=\"bu\"><br><br>\
  <label for=\"bw\">Password:</label>\
  <input type=\"password\" id=\"bw\" name=\"bw\"><br><br>\
  \
 </fieldset>\
 <input type=\"submit\" value=\"Save\">\
</form>\
</body>\
</html>";
    httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void urldecode2(char *dst, const char *src)
{
    char a, b;
    while (*src)
    {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

bool parse_and_set_config(b2mconfig_t *b2mconfig, char *content)
{
    char delimit[] = "&=";

    char *token = strtok(content, delimit);
    char **config = NULL;
    int config_size = 10;
    while (token != NULL)
    {
        if (strncmp(token, "ws", 2) == 0)
        {
            config = &b2mconfig->wifi_ssid;
            config_size = 20;
        }
        else if (strncmp(token, "wp", 2) == 0)
        {
            config = &b2mconfig->wifi_password;
            config_size = 30;
        }
        else if (strncmp(token, "bi", 2) == 0)
        {
            config = &b2mconfig->broker_ip_address;
            config_size = 17;
        }
        else if (strncmp(token, "bp", 2) == 0)
        {
            config = &b2mconfig->broker_port;
            config_size = 6;
        }
        else if (strncmp(token, "bu", 2) == 0)
        {
            config = &b2mconfig->broker_username;
            config_size = 20;
        }
        else if (strncmp(token, "bw", 2) == 0)
        {
            config = &b2mconfig->broker_password;
            config_size = 20;
        }
        else
        {
            ESP_LOGW(TAG, "Unknown token: %s", token);
            return false;
        }

        ESP_LOGI(TAG, "token: %s. alloc memory", token);

        *config = calloc((sizeof(char) * config_size) + 1, 1);
        if (*config == NULL)
        {
            ESP_LOGE(TAG, "Out of memory");
            return false;
        }

        token = strtok(NULL, delimit);
        if (token == NULL)
        {
            return false;
        }
        if (strlen(token) > config_size)
        {
            ESP_LOGE(TAG, "Token length is too long (current: %d, max: %d)", strlen(token), config_size);
            return false;
        }
        urldecode2(*config, token);
        //strncpy(*config, token, config_size);
        token = strtok(NULL, delimit);
    }
    ESP_LOGI(TAG, "wifi_ssid: \'%s\'", b2mconfig->wifi_ssid);
    ESP_LOGI(TAG, "wifi_password: \'%s\'", b2mconfig->wifi_password);
    ESP_LOGI(TAG, "broker_ip_address: \'%s\'", b2mconfig->broker_ip_address);
    ESP_LOGI(TAG, "broker_port: \'%s\'", b2mconfig->broker_port);
    ESP_LOGI(TAG, "broker_username: \'%s\'", b2mconfig->broker_username);
    ESP_LOGI(TAG, "broker_password: \'%s\'", b2mconfig->broker_password);

    return true;
}

esp_err_t post_save_handler(httpd_req_t *req)
{
#define MAX_RESPONSE_SIZE (200)
    char *content = calloc(sizeof(char) * MAX_RESPONSE_SIZE, 1);
    if (content == NULL)
    {
        ESP_LOGE(TAG, "Out of memory");
        return ESP_FAIL;
    }

    size_t recv_size = req->content_len > MAX_RESPONSE_SIZE ? MAX_RESPONSE_SIZE : req->content_len;

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    bool result = parse_and_set_config(req->user_ctx, content);
    free(content);
    content = NULL;

    if (result == true)
    {
        const char resp[] = "<html><body><p>The configuration has been completed. Restart the device.</p></body></html>";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        b2mconfig_save(req->user_ctx);
    }
    else
    {
        const char resp[] = "<html><body><p>An error occurred while saving the configuration. Restart the device.</p></body></html>";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

httpd_handle_t start_webserver(b2mconfig_t *b2mconfig)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_resp_headers = 4;
    httpd_handle_t server = NULL;

    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = (void *)b2mconfig};

    httpd_uri_t uri_save_post = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = post_save_handler,
        .user_ctx = (void *)b2mconfig};

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_save_post);
    }
    return server;
}

void initialise_mdns(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(WIFI_AP_SSID));
    ESP_ERROR_CHECK(mdns_instance_name_set(WIFI_AP_SSID));
}

void vTaskB2MConfig(void *pvParameters)
{
    b2mconfig_t *b2mconfig = (b2mconfig_t *)pvParameters;
    ESP_LOGI(TAG, "Run task b2mconfig");
    EventGroupHandle_t s_event_group = xEventGroupCreate();
    EventBits_t bits;

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();

    esp_netif_t *esp_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    ESP_ERROR_CHECK(esp_netif_set_hostname(esp_netif, WIFI_AP_SSID));

    initialise_mdns();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &event_ap_handler,
                                               (void *)&s_event_group));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_AP_STAIPASSIGNED,
                                               &event_ap_handler,
                                               (void *)&s_event_group));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASSWORD,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = 6,
            .max_connection = 1},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    httpd_handle_t http_server = start_webserver(b2mconfig);

    while (1)
    {
#ifndef DISABLETASKMSG
        ESP_LOGD(TAG, "Task b2mconfig");
#endif

        if (!(xEventGroupGetBits(s_event_group) & TB2MCONFIG_WIFI_CLIENT_CONNECTED_BIT))
        {
            bits = xEventGroupWaitBits(s_event_group, TB2MCONFIG_WIFI_CLIENT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY); //Wait for wifi
            if (!(bits & TB2MCONFIG_WIFI_CLIENT_CONNECTED_BIT))
                continue;
        }
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}