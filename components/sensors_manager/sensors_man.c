#include "sensors_man.h"

esp_err_t i2c_init_sensors() {
    esp_err_t ret = ESP_OK;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if(ret != ESP_OK){
        ESP_LOGE(SENSORS_TAG, "Setting i2c config failed: %d", ret);
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

    if(ret != ESP_OK){
        ESP_LOGE(SENSORS_TAG, "i2c_driver_install failed: %d", ret);
    }

    return ret;
}


void i2c_scanner() {
    int i;
    esp_err_t espRc;

    printf(">> I2C scanning ...\n\n");
    for (i = 3; i < 0x78; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        espRc = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
        if (espRc == 0) {
            printf("Found I2C device at address 0x%02x\n", i);
        }

        i2c_cmd_link_delete(cmd);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    printf("\n>> Scanning done.\n");
}

 esp_err_t mic_setup(enum Microphone mic_type, i2s_chan_handle_t* rx_handle) {
    esp_err_t err = ESP_OK;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    err = i2s_new_channel(&chan_cfg, NULL, rx_handle);
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error creating I2S channel: %s",  esp_err_to_name(err));
        return err;
    }

    switch (mic_type) {
        case INMP441:
            ESP_LOGI(SENSORS_TAG, "Initializing INMP441");
            i2s_std_config_t std_cfg = {
                .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
                .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
                .gpio_cfg = {
                    .mclk = I2S_GPIO_UNUSED,
                    .bclk = GPIO_NUM_4,
                    .ws = GPIO_NUM_5,
                    .dout = I2S_GPIO_UNUSED,
                    .din = GPIO_NUM_19,
                    .invert_flags = {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv = false,
                    },
                },
            };

            err = i2s_channel_init_std_mode(*rx_handle, &std_cfg);
            if(err != ESP_OK) {
                ESP_LOGE(SENSORS_TAG, "Error initializing I2S channel: %s",  esp_err_to_name(err));
                return err;
            }

            err = i2s_channel_enable(*rx_handle);
            if(err != ESP_OK) {
                ESP_LOGE(SENSORS_TAG, "Error enabling I2S channel: %s", esp_err_to_name(err));
                return err;
            }
            break;
        
        default:
            ESP_LOGE(SENSORS_TAG, "Failed to initialize microphone: Unknown hardware");
            return ESP_ERR_INVALID_ARG;
    }

    return err;
}

 esp_err_t read_noise_level(i2s_chan_handle_t* rx_handle, int* data) {
    esp_err_t err = ESP_OK;

    //ESP_LOGI(SENSORS_TAG, "Reading noise level");
    err = i2s_channel_read(*rx_handle, data, 4, NULL, 1000);
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error reading I2S data: %s",  esp_err_to_name(err));
    }

    return err;
}

esp_err_t mic_shut_down(i2s_chan_handle_t* rx_handle) {
    esp_err_t err = ESP_OK;

    err = i2s_channel_disable(*rx_handle);
    if(err != ESP_OK) {
        return err;
    }

    err = i2s_del_channel(*rx_handle);

    return err;
}

esp_err_t init_PmodHYGRO() {
    esp_err_t ret = ESP_OK;
    uint8_t data[2];
    uint8_t reg_addr = 0x00;

    ret = i2c_master_write_to_device(0, 0x40, &reg_addr, 1, 10000 / portTICK_PERIOD_MS);
    if(ret != ESP_OK){
        ESP_LOGE(SENSORS_TAG, "Initializing PmodHYGRO failed: %d", ret);
    }

    return ret;
}

esp_err_t PmodHYGRO_read(int *temp, int *hum) {
    esp_err_t ret = ESP_OK;
    uint8_t data[4];
    uint8_t reg_addr = 0x00;

    i2c_master_write_to_device(0, 0x40, &reg_addr, 1, 10000 / portTICK_PERIOD_MS);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    ret = i2c_master_read_from_device(0, 0x40, data, 4, 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Reading from PmodHYGRO failed: %d", ret);
        return ret;
    }

    uint16_t rawTemp = (data[0] << 8) | data[1];
    uint16_t rawHum = (data[2] << 8) | data[3];

    *temp = ((rawTemp / 65536.0) * 165) - 40;
    *hum = (rawHum / 65536.0) * 100;

    return ret;
}

esp_err_t PmodTMP3_read(int *temp) {
    esp_err_t ret = ESP_OK;
    uint8_t data[1];

    ret = i2c_master_read_from_device(0, 0x48, data, 1, 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "i2c_master_read_from_device failed: %d", ret);
        return ret;
    }

    uint16_t rawTemp = (data[0] << 4) | data[0];

    *temp = rawTemp/16;

    return ret;
}