#include "sensors_man.h"

esp_err_t i2c_setup(i2c_port_t port, i2c_mode_t mode, gpio_num_t scl, gpio_num_t sda, uint32_t clk_freq) {
    esp_err_t err = ESP_OK;

    i2c_config_t conf = {
        .mode = mode,
        .sda_io_num = sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_freq
    };

    err = i2c_param_config(port, &conf);
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error configuring I2C driver: %d", err);
        return err;
    }
    
    err = i2c_driver_install(port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if(err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Error installing I2C driver: %d", err);
        return err;
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

esp_err_t read_audio(float* noise) {
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


esp_err_t SHT40_read(i2c_port_t port, float* temp, float* hum) {
    esp_err_t err = ESP_OK;
    uint8_t r_data[6];
    uint8_t w_data = 0xFD;
    uint16_t t_ticks, rh_ticks;

    vTaskDelay(500 / portTICK_PERIOD_MS);

    err = i2c_master_write_read_device(port, 0x44, w_data, sizeof(w_data), r_data, sizeof(r_data), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    if (err != ESP_OK) {
        ESP_LOGE(SENSORS_TAG, "Reading from SHT40 failed: %d", err);
        return err;
    }

    t_ticks = r_data[0] * 256 + r_data[1];
    rh_ticks = r_data[3] * 256 + r_data[4];
    
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