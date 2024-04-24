#ifndef SENSORS_MAN_H
#define SENSORS_MAN_H

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include <driver/i2s_std.h>
#include <driver/gpio.h>

#define I2C_MASTER_SCL_IO 22   
#define I2C_MASTER_SDA_IO 21    
#define I2C_SENSORS_PORT I2C_NUM_1
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_DEFAULT_FREQ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TIMEOUT_MS 1000
#define SAMPLE_SIZE 6000

#define SENSORS_TAG "SENSORS_MANAGER"

typedef enum {
    INMP441
} Microphone;

typedef struct {
    i2c_master_bus_handle_t i2c_bus_handle;
    i2c_master_dev_handle_t sensor_handle;
    uint8_t write_data;
} I2C_Sensor;

esp_err_t i2c_init(i2c_master_bus_handle_t* i2c_bus_handle);

void i2c_scanner();

/**
 * @brief Set up microphone, including I2S protocol and specific hardware
 * 
 * @param mic_type Microphone hardware being used
 * @param rx_handle Handler variable for I2S module on ESP32
 * 
 * @return esp_err_t Error code
*/
esp_err_t mic_setup(Microphone mic_type, i2s_chan_handle_t* rx_handle);

/**
 * @brief Reads the current noise level
 * 
 * @param rx_handle Handler variable for I2S module on ESP32
 * @param i2s_readraw_buff Destination buffer for read data
 * @param bytes_read Pointer to store real amount of read data
 * 
 * @return esp_err_t Error code
*/
esp_err_t read_noise_level(i2s_chan_handle_t* rx_handle, int32_t* i2s_readraw_buff, size_t* bytes_read);

/**
 * @brief Future function intended for recording audio samples of hives for post analysis and model training
 * 
 * @param rx_handle Handler variable for I2S module on ESP32
 * 
 * @return esp_err_t Error code
*/
//esp_err_t record_sample(i2s_chan_handle_t* rx_handle);

/**
 * @brief Shuts down the microphone module
 * 
 * @param rx_handle Handler variable for I2S module on ESP32
 * 
 * @return esp_err_t Error code
*/
esp_err_t mic_shut_down(i2s_chan_handle_t* rx_handle);

/**
 * @brief Initialize SHT40 sensor
 * 
 * @return esp_err_t Error code
*/
esp_err_t SHT40_init(I2C_Sensor *sht40);

/**
 * @brief Read values from SHT40 sensor
 * 
 * @param sht40 Pointer to i2c sensor struct
 * 
 * @return esp_err_t Error code
*/
esp_err_t SHT40_read(I2C_Sensor *sht40, float *temp, float *hum);

#endif