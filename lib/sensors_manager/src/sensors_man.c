#include "sensors_man.h"

esp_err_t i2c_bus_init(i2c_master_bus_handle_t* i2c_bus_handle, i2c_port_num_t port, int sda, int scl) {
    esp_err_t err = ESP_OK;

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = port,
        .scl_io_num = scl,
        .sda_io_num = sda,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    err = i2c_new_master_bus(&i2c_mst_config, i2c_bus_handle);
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error creating I2C master bus: %d", err);
    }
    
    return err;
}

esp_err_t mic_setup(Microphone mic_type) {
    esp_err_t err = ESP_OK;
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error creating I2S channel: %s",  esp_err_to_name(err));
        return err;
    }

    switch (mic_type) {
        case INMP441:
            ESP_LOGI(SENSORS_TAG, "Initializing INMP441");
              i2s_config_t i2s_config = {
                .mode = I2S_MODE_MASTER | I2S_MODE_RX,
                .sample_rate = 44100,
                .bits_per_sample = I2S_BITS_PER_SAMPLE_24BIT,
                .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
                .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                .intr_alloc_flags = 0,
                .dma_buf_count = 8,
                .dma_buf_len = bufferLen,
                .use_apll = false
              };

            err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
            if(err != ESP_OK) {
                ESP_LOGE(SENSORS_TAG, "Error installing I2S channel: %s",  esp_err_to_name(err));
                return err;
            }

            i2s_pin_config_t pin_config = {
                .bck_io_num = GPIO_NUM_4,
                .ws_io_num = GPIO_NUM_5,
                .data_out_num = -1,
                .data_in_num = GPIO_NUM_19
            };

            i2s_set_pin(I2S_NUM_0, &pin_config);
            if(err != ESP_OK) {
                ESP_LOGE(SENSORS_TAG, "Error configuring I2S channel pins: %s", esp_err_to_name(err));
                return err;
            }
            break;
        
        default:
            ESP_LOGE(SENSORS_TAG, "Failed to initialize microphone: Unknown hardware");
            return ESP_ERR_INVALID_ARG;
    }

    return err;
}

esp_err_t read_noise_level(float* noise) {
    esp_err_t err = ESP_OK;
    int32_t sBuffer[bufferLen];
    size_t bytesIn;
    int32_t i, samples_read;

    *noise = 0;
    ESP_LOGI(SENSORS_TAG, "Starting read operation");
    err = i2s_read(I2S_NUM_0, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error reading I2S data: %s",  esp_err_to_name(err));
        return err;
    }

    samples_read = bytesIn / sizeof(int32_t);

    //RMS for dB measuring
    for(i = 0; i < samples_read; ++i){
        *noise += pow(sBuffer[i], 2);
    }

    *noise /= samples_read;
    *noise = sqrt(*noise);

    return err;
}

esp_err_t mic_shut_down(i2s_chan_handle_t* rx_handle) {
    esp_err_t err = ESP_OK;

    //err = i2s_channel_disable(*rx_handle);
    if(err != ESP_OK) {
        return err;
    }

    //err = i2s_del_channel(*rx_handle);

    return err;
}

esp_err_t SHT40_init(I2C_Sensor *sht40) {
    esp_err_t err = ESP_OK;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = 0x44,
        .scl_speed_hz = I2C_DEFAULT_FREQ
    };

    err = i2c_master_bus_add_device(*sht40->i2c_bus_handle, &dev_cfg, &sht40->sensor_handle);
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
    
    *temp = -45 + 175 * t_ticks/65535.;
    *hum = -6 + 125 * rh_ticks/65536.;

    return err;
}

esp_err_t HX711_init(Sensor hx771) {
    esp_err_t err = ESP_OK;
    
    gpio_config_t hx771_conf ={
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<hx771.sck),
        .pull_down_en = 0,
        .pull_up_en = 0
    };

    err = gpio_config(&hx771_conf);
    if(err != ESP_OK){
        ESP_LOGE(SENSORS_TAG, "Failed to initialize HX771 when configuring SCK: %d", err);
        return err;
    }

    hx771_conf.intr_type = GPIO_INTR_DISABLE;
    hx771_conf.pin_bit_mask = (1ULL<<hx771.sda);
    hx771_conf.mode = GPIO_MODE_INPUT;
    hx771_conf.pull_up_en = 0;

    err = gpio_config(&hx771_conf);
    if(err != ESP_OK){
        ESP_LOGE(SENSORS_TAG, "Failed to initialize HX771 when configuring Data input: %d", err);
    }

    return err;
}

esp_err_t HX711_read(Sensor hx771, float *weight) {
    esp_err_t err = ESP_OK;
    int i;
    char j;
    unsigned long value, sum = 0;

    for(j = 0; j < 10; j++) {
        gpio_set_level(hx771.sck, 0);

    	while (gpio_get_level(hx771.sda)) {
    		vTaskDelay(10 / portTICK_PERIOD_MS);
    	}

        value = 0;

        portDISABLE_INTERRUPTS();

        for(i = 0; i < 24 ; i++) {   
    		gpio_set_level(hx771.sck, 1);
            vTaskDelay(20 / portTICK_PERIOD_MS);
            value = value << 1;
            gpio_set_level(hx771.sck, 0);
            vTaskDelay(20 / portTICK_PERIOD_MS);

            if(gpio_get_level(hx771.sda))
            	value++;
    	}

		gpio_set_level(hx771.sck, 1);
		vTaskDelay(20 / portTICK_PERIOD_MS);
		gpio_set_level(hx771.sck, 0);
		vTaskDelay(20 / portTICK_PERIOD_MS);

    	portENABLE_INTERRUPTS();

        sum += value^0x800000;
    }

    *weight = (sum/10.) / TARE;

    return err;
}