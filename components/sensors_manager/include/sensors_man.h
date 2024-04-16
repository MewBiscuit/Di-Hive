#ifndef SENSORS_MAN_H
#define SENSORS_MAN_H

#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <driver/i2s_std.h>
#include <driver/gpio.h>

#define I2C_MASTER_SCL_IO    22   
#define I2C_MASTER_SDA_IO    21    
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000

#define SENSORS_TAG "SENSORS_MANAGER"
enum Microphone {
    INMP441
};

esp_err_t i2c_init_sensors();

void i2c_scanner();

/**
 * @brief Set up microphone, including I2S protocol and specific hardware
 * 
 * @param mic_type Microphone hardware being used
 * @param rx_handle Handler variable for I2S module on ESP32
 * 
 * @return esp_err_t Error code
*/
esp_err_t mic_setup(enum Microphone mic_type, i2s_chan_handle_t* rx_handle);

/**
 * @brief Reads the current noise level
 * 
 * @param rx_handle Handler variable for I2S module on ESP32
 * 
 * @return 
*/
esp_err_t read_noise_level(i2s_chan_handle_t* rx_handle);

/**
 * @brief Future function intended for recording audio samples of hives for post analysis and model training
 * 
 * @param rx_handle Handler variable for I2S module on ESP32
 * 
 * @return esp_err_t Error code
*/
esp_err_t record_sample(i2s_chan_handle_t* rx_handle);


esp_err_t init_PmodHYGRO();

esp_err_t PmodHYGRO_read(int *temp, int *hum);

esp_err_t PmodTMP3_read(int *temp);



#endif