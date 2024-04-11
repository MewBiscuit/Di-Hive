#ifndef SENSORS_MAN_H
#define SENSORS_MAN_H

#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <driver/i2s.h>

#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_26
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_22
#define I2S_MIC_SERIAL_DATA GPIO_NUM_21

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
 * 
 * @return esp_err_t Error code
*/
esp_err_t mic_setup(enum Microphone mic_type);


esp_err_t init_PmodHYGRO();

esp_err_t PmodHYGRO_read(int *temp, int *hum);

esp_err_t PmodTMP3_read(int *temp);



#endif