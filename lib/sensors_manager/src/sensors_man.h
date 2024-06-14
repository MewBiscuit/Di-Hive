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
#include "driver/i2c.h"
#include <driver/i2s.h>
#include <driver/gpio.h>

#include "hx711.h"

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

/**
 * @brief Initializes I2C module with specified parameters
 * 
 * @param port Internal port selected for I2C
 * @param sda Data GPIO of bus
 * @param mode Selected mode for ESP32
 * @param scl Clock GPIO of bus 
 * @param clk_freq Clock frequency of I2C
 * 
 * @return esp_err_t Error code
*/
esp_err_t i2c_setup(i2c_port_t port, i2c_mode_t mode, gpio_num_t scl, gpio_num_t sda, uint32_t clk_freq);

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
 * @param noise RMS of sound from read samples
 * 
 * @return esp_err_t Error code
*/
esp_err_t read_audio(float* noise);

/**
 * @brief Future function intended for recording audio samples of hives for post analysis and model training
 * 
 * @param noise Average of sound from read samples
 * 
 * @return esp_err_t Error code
*/
//esp_err_t record_sample(i2s_chan_handle_t* rx_handle);

/**
 * @brief Read values from SHT40 sensor
 * 
 * @param temp Pointer to temperature variable
 * @param hum Pointer to humidity variable
 * 
 * @return esp_err_t Error code
*/
esp_err_t SHT40_read(i2c_port_t port, float* temp, float* hum);

/**
 * @brief Initialize HX711 sensor with clock, data, channel and gain factor
 * 
 * @param hx711 Struct containing SDA and SCL pin numbers
 * 
 * @return esp_err_t Error code
*/
esp_err_t HX711_init(hx711_t *sensor);

/**
 * @brief Read values and transfrom to weight from HX771 sensor
 * 
 * @param hx711 Struct containing SDA and SCL pin numbers
 * @param weight Pointer to weight variable
 * 
 * @return esp_err_t Error code
*/
esp_err_t HX711_read(hx711_t *sensor, float* weight, int32_t *raw_data);

#endif