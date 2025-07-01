#include "i2c.h"
#include "../../defines.h"

esp_err_t i2c_master_init(void) {
    i2c_config_t conf;
    memset(&conf, 0, sizeof(i2c_config_t));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA_I2C_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = SCL_I2C_PIN;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

void deinit_i2c(void) {
    esp_err_t ret = i2c_driver_delete(I2C_MASTER_NUM);
    if (ret != ESP_OK) {
        Serial.printf("i2c_driver_delete error: %s\r\n", esp_err_to_name(ret));
    } else {
        Serial.printf("I2C driver deleted successfully.\r\n");
    }
}