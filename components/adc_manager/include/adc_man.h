#ifndef ADC_MAN_H
#define ADC_MAN_H

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *ADC_TAG = "adc_manager";

/**
 * @brief Initialize the adc module
 *
 * @param adc_module ADC module enum (ADC_UNIT_1 or ADC_UNIT_2)
 * 
 * @return adc_oneshot_unit_handle_t Handle to the adc module
*/
adc_oneshot_unit_handle_t adc_manager_init(adc_unit_t adc_module);

/**
 * @brief Configure the adc channel
 *
 * @param adc_module ADC module handle
 * @param channel  ADC channel enum
 * @param bitwidth ADC bitwidth enum
 * @param atten ADC attenuation enum
 * 
 * @return esp_err_t
*/
esp_err_t adc_manager_cfg_channel(adc_oneshot_unit_handle_t adc_module, adc_channel_t channel, adc_bitwidth_t bitwidth, adc_atten_t atten);


/**
 * @brief Read the adc value
 *
 * @param adc_module ADC module handle
 * @param channel ADC channel enum
 * @param data Pointer to the data variable to store the adc value
 * 
 * @return esp_err_t
*/
esp_err_t adc_manager_read_oneshot(adc_oneshot_unit_handle_t adc_module, adc_channel_t channel, int* data);

/**
 * @brief Deinitialize the adc module
 * 
 * @param adc_module ADC module handle
 * 
 * @return esp_err_t
*/
esp_err_t adc_manager_deinit(adc_oneshot_unit_handle_t adc_module);

#endif