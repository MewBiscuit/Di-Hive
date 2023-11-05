#include <stdio.h>
#include <string.h>

#include "wifi_manager.h"

void app_main(void)
{
    // Let's test the wifi module
    wifi_init();
    char *ssid = "MyAP";
    char *password = "MyPassword123";
    int channel = 1;
    int max_connections = 4;
    setup_ap(ssid, password, &channel, &max_connections);

    vTaskDelay(100000 / portTICK_PERIOD_MS);
}
