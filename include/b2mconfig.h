#ifndef HEADER_B2MCONFIG
#define HEADER_B2MCONFIG

typedef struct b2mconfig_def
{
    bool not_initialized;

    char *wifi_ssid;
    char *wifi_password;

    char *broker_ip_address;
    uint32_t broker_port;
    char *broker_username;
    char *broker_password;
} b2mconfig_t;

#endif