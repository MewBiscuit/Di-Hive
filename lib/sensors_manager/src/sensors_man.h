#ifndef SENSORS_MAN_H
#define SENSORS_MAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/i2c_master.h"
#include <driver/i2s.h>
#include <driver/gpio.h>
 
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_DEFAULT_FREQ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TIMEOUT_MS 1000
#define SAMPLE_SIZE 6000
#define TARE 1
#define bufferLen 64

#define SENSORS_TAG "SENSORS_MANAGER"

typedef enum {
    INMP441
} Microphone;

typedef struct {
    i2c_master_bus_handle_t* i2c_bus_handle;
    i2c_master_dev_handle_t sensor_handle;
    uint8_t write_data;
} I2C_Sensor;

typedef struct{
    gpio_num_t sda;
    gpio_num_t sck;
} Sensor;

/**
 * @brief Initializes an I2C bus with specified parameters
 * 
 * @param i2c_bus_handle  ESP-IDF handler structure for I2C bus
 * @param port Internal port selected for I2C bus
 * @param sda Data GPIO of bus
 * @param scl Clock GPIO of bus
 * 
 * @return esp_err_t Error code
*/
esp_err_t i2c_bus_init(i2c_master_bus_handle_t* i2c_bus_handle, i2c_port_num_t port, int sda, int scl);

void i2c_scanner();

/**
 * @brief Set up microphone, including I2S protocol and specific hardware
 * 
 * @param mic_type Microphone hardware being used
 * 
 * @return esp_err_t Error code
*/
esp_err_t mic_setup(Microphone mic_type);

/**
 * @brief Reads the current noise level
 * 
 * @param rx_handle Handler variable for I2S module on ESP32
 * 
 * @return esp_err_t Error code
*/
esp_err_t read_noise_level(float* noise);

/**
 * @brief Future function intended for recording audio samples of hives for post analysis and model training
 * 
 * @param noise Average of sound from read samples
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
 * @param sht40 Pointer to I2C sensor struct
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

/**
 * @brief Initialize HX711 sensor with clock, data, channel and gain factor
 * 
 * @param hx711 Struct containing SDA and SCL pin numbers
 * 
 * @return esp_err_t Error code
*/
esp_err_t HX711_init(Sensor hx771);

/**
 * @brief Read values and transfrom to weight from HX771 sensor
 * 
 * @param hx711 Struct containing SDA and SCL pin numbers
 * @param weight Pointer to weight variable
 * 
 * @return esp_err_t Error code
*/
esp_err_t HX711_read(Sensor hx771, float* weight);

#endif