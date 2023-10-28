#include <stdio.h>
#include <string.h>
#include "wifi_manager.h"
#include "web_provisioning.h"
#include "nvs_manager.h"
#include "web_example.h"

/* Wi-Fi credentials */
char* ssid = "";
char* password = "";

void app_main(void)
{
    //Variables
    int result = -1;

    //Initialize NVS
    init_nvs();

    while(result != 0)
    {
        //Check for saved Wi-Fi credentials
        if (read_string_from_nvs("ssid", ssid) == ESP_OK && read_string_from_nvs("password", password) == ESP_OK)
        {
            printf("Saved Wi-Fi credentials found\n");
            printf("SSID: %s\n", ssid);
            printf("Attempting to connect to Wi-Fi\n");

            //Connect to Wi-Fi
            result = wifi_connect_ap(ssid, password);
        }

        //If no Wi-Fi credentials are saved or connection failed, set up access point
        if(result != 0)
        {
            printf("Could not connect to Wi-Fi\n");
            printf("Select provisioning mode: 1 for Wi-Fi, 2 for Bluetooth\n");
            scanf("%d", &result);

            //Set up Wi-Fi access point
            if(result == 1)
            {
                printf("Setting up Wi-Fi access point\n");
                wifi_setup_ap("Eu Sonno ESP-32", "123abcdef");
            }
        }
    }

    //Start web server for data visualization
    start_web_server();
    

}
