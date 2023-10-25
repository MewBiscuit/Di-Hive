#include <stdio.h>
#include <string.h>
#include "esp_bt.h"
#include "nvs_manager.h"

#define MAX_FILE_SIZE 1024

void receive_file_over_bluetooth() {
    // Initialize Bluetooth
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    esp_bluedroid_init();
    esp_bluedroid_enable();
    esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);

    // Wait for a Bluetooth connection to be established
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0, 0);

    // Receive the file over Bluetooth
    char file_data[MAX_FILE_SIZE];
    int file_size = esp_spp_receive(file_data, MAX_FILE_SIZE);

    // Parse the received file to extract the SSID and password
    char ssid[32];
    char password[64];
    sscanf(file_data, "%s %s", ssid, password);

    // Save the extracted credentials to nvs
    init_nvs();
    write_string_to_nvs("ssid", ssid);
    write_string_to_nvs("password", password);
}
