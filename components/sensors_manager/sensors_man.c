#include "sensors_man.h"

esp_err_t i2c_init(i2c_master_bus_handle_t* i2c_bus_handle) {
    esp_err_t err = ESP_OK;

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_SENSORS_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    err = i2c_new_master_bus(&i2c_mst_config, i2c_bus_handle);
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error creating I2C master bus: %d", err);
    }
    
    return err;
}

esp_err_t mic_setup(Microphone mic_type, i2s_chan_handle_t* rx_handle) {
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
                .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_MONO),
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

esp_err_t read_noise_level(i2s_chan_handle_t* rx_handle, int32_t* i2s_readraw_buff, size_t* bytes_read) {
    esp_err_t err = ESP_OK;

    err = i2s_channel_read(*rx_handle, i2s_readraw_buff, 1024, bytes_read, I2C_MASTER_TIMEOUT_MS);
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

esp_err_t SHT40_init(I2C_Sensor *sht40) {
    esp_err_t err = ESP_OK;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = 0x44,
        .scl_speed_hz = I2C_DEFAULT_FREQ,
    };

    err = i2c_master_bus_add_device(sht40->i2c_bus_handle, &dev_cfg, &sht40->sensor_handle);
    if(err != ESP_OK){
        ESP_LOGE(SENSORS_TAG, "Initializing SHT40 failed when adding to bus: %d", err);
    }

    return err;
}

esp_err_t SHT40_read(I2C_Sensor *sht40, float* temp, float* hum) {
    esp_err_t err = ESP_OK;
    uint8_t data[6];
    uint16_t t_ticks, rh_ticks;

    err = i2c_master_transmit(sht40->sensor_handle, &sht40->write_data, 1, 10000);
    if(err != ESP_OK){
        ESP_LOGE(SENSORS_TAG, "Reading SHT40 failed during write: %d", err);
        return err;
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);

    err = i2c_master_receive(sht40->sensor_handle, data, 6, 10000);
    if (err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Reading from SHT40 failed during read: %d", err);
        return err;
    }

    t_ticks = data[0] * 256 + data[1];
    rh_ticks = data[3] * 256 + data[4];
    
    *temp = -45 + 175 * t_ticks/65535;
    *hum = -6 + 125 * rh_ticks/65536;

    return err;
}