#include "i2c.h"
#include "../../defines.h"

static SemaphoreHandle_t i2c_bus_mutex = nullptr;
static bool i2c_bus_initialized = false;

static esp_err_t i2c_install_driver_once(void) {
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

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                             I2C_MASTER_RX_BUF_DISABLE,
                             I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
        return ESP_OK;
    }
    return err;
}

esp_err_t i2c_bus_init(void) {
    if (i2c_bus_initialized) {
        return ESP_OK;
    }

    if (!i2c_bus_mutex) {
        i2c_bus_mutex = xSemaphoreCreateMutex();
        if (!i2c_bus_mutex) {
            return ESP_ERR_NO_MEM;
        }
    }

    esp_err_t err = i2c_install_driver_once();
    if (err != ESP_OK) {
        return err;
    }

    i2c_bus_initialized = true;
    return ESP_OK;
}

bool i2c_bus_lock(TickType_t timeout_ticks) {
    if (!i2c_bus_mutex) {
        i2c_bus_mutex = xSemaphoreCreateMutex();
        if (!i2c_bus_mutex) {
            return false;
        }
    }

    if (xSemaphoreTake(i2c_bus_mutex, timeout_ticks) != pdTRUE) {
        return false;
    }

    if (!i2c_bus_initialized) {
        esp_err_t err = i2c_install_driver_once();
        if (err != ESP_OK) {
            xSemaphoreGive(i2c_bus_mutex);
            return false;
        }
        i2c_bus_initialized = true;
    }

    return true;
}

void i2c_bus_unlock(void) {
    if (i2c_bus_mutex) {
        xSemaphoreGive(i2c_bus_mutex);
    }
}

I2cBusLock::I2cBusLock(TickType_t timeout_ticks) {
    held_ = i2c_bus_lock(timeout_ticks);
}

I2cBusLock::~I2cBusLock() {
    if (held_) {
        i2c_bus_unlock();
    }
}

esp_err_t i2c_master_init(void) {
    return i2c_bus_init();
}

void deinit_i2c(void) {
    // Shared bus stays installed for the lifetime of the device.
}
