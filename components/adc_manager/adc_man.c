 #include "adc_man.h"

adc_oneshot_unit_handle_t adc_manager_init(adc_unit_t adc_module) {
    //Setup ADC
    adc_oneshot_unit_handle_t adc_handle;

    // Setup ADC1
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = adc_module,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    return adc_handle;
}

esp_err_t adc_manager_cfg_channel(adc_oneshot_unit_handle_t adc_module, adc_channel_t channel, adc_bitwidth_t bitwidth, adc_atten_t atten) {
    //Configure ADC
    esp_err_t r = ESP_OK;
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = bitwidth,
        .atten = atten,
    };

    r = adc_oneshot_config_channel(adc_module, channel, &config);
    return r;
}

esp_err_t adc_manager_read_oneshot(adc_oneshot_unit_handle_t adc_module, adc_channel_t channel, int* data) {
    //Read ADC
    esp_err_t r = adc_oneshot_read(adc_module, channel, data);
    if(r != ESP_OK) {
        ESP_LOGE(ADC_TAG, "ADC read error");
        return r;
    }
    return r;
}

esp_err_t adc_manager_deinit(adc_oneshot_unit_handle_t adc_module) {
    //Deinitialize ADC
    esp_err_t r = adc_oneshot_del_unit(adc_module);
    if(r != ESP_OK) {
        ESP_LOGE(ADC_TAG, "ADC deinit error");
        return r;
    }
    return r;
}