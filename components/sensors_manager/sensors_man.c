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

esp_err_t mic_setup(enum Microphone mic_type) {
    esp_err_t err = ESP_OK;

    switch (mic_type) {
        case INMP441:
                i2s_config_t i2s_config = {
                    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
                    .sample_rate = 48000,
                    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
                    .communication_format = I2S_COMM_FORMAT_I2S,
                    .intr_alloc_flags = 0,
                    .dma_buf_count = 2,
                    .dma_buf_len = 1024
                };

                err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
        
        default:
            return ESP_ERR_INVALID_ARG;
    }

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