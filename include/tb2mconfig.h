#ifndef HEADER_TCONFIG
#define HEADER_TCONFIG

#include "b2mconfig.h"

b2mconfig_t* b2mconfig_create(void);
void b2mconfig_load(b2mconfig_t *b2mconfig);

void vTaskB2MConfig(void *pvParameters);
#endif