#ifndef __I2C_H__
#define __I2C_H__

#include "driver/i2c.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define I2C_MASTER_FREQ_HZ         100000
#define I2C_MASTER_TX_BUF_DISABLE  0
#define I2C_MASTER_RX_BUF_DISABLE  0
#define I2C_MASTER_NUM             I2C_NUM_0

/** Install the shared I2C driver once (idempotent). */
esp_err_t i2c_bus_init(void);

/** Acquire the shared I2C bus mutex (lazy-inits the driver on first success). */
bool i2c_bus_lock(TickType_t timeout_ticks = portMAX_DELAY);

/** Release the shared I2C bus mutex. */
void i2c_bus_unlock(void);

/**
 * RAII guard for shared I2C bus access.
 * Sensor drivers should hold this for the duration of begin()/fetch() I2C work.
 */
class I2cBusLock {
public:
    explicit I2cBusLock(TickType_t timeout_ticks = portMAX_DELAY);
    ~I2cBusLock();

    bool ok() const { return held_; }

    I2cBusLock(const I2cBusLock&) = delete;
    I2cBusLock& operator=(const I2cBusLock&) = delete;

private:
    bool held_ = false;
};

/** @deprecated Use i2c_bus_init() / I2cBusLock instead. */
esp_err_t i2c_master_init(void);

/** @deprecated Bus stays up for device lifetime; no-op. */
void deinit_i2c(void);

#endif  // __I2C_H__
