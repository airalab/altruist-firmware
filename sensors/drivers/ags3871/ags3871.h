#ifndef __AGS3871_DRIVER_H__
#define __AGS3871_DRIVER_H__

#include <stdint.h>
#include "driver/i2c.h"
#include "../i2c.h"

enum AGS3871Error
{
    AGS3871_OK = 0,
    AGS3871_ERROR_I2C = -1,
    AGS3871_ERROR_READ = -2,
    AGS3871_ERROR_CRC = -3,
    AGS3871_ERROR_NOT_READY = -4,
};

struct AGS3871RegisterData
{
    // Raw payload returned by every readable register: Data1..Data4.
    uint8_t data[4] = {0, 0, 0, 0};
    // CRC byte returned by the sensor. CRC is calculated over Data1..Data4.
    uint8_t crc = 0;
    bool crc_valid = false;
};

struct AGS3871ConcentrationData
{
    // Data1 from register 0x00. Bit 0 is RDY: 0 = new data ready, 1 = not ready/warmup.
    uint8_t status = 0;
    bool ready = false;
    // 24-bit CO concentration from Data2..Data4, in ppm according to the datasheet.
    uint32_t ppm = 0;
};

class AGS3871Driver
{
public:
    // Datasheet I2C address is 0x1A in 7-bit form.
    static constexpr uint8_t ADDRESS = 0x1A;
    static constexpr uint8_t REG_CONCENTRATION = 0x00;
    static constexpr uint8_t REG_ZERO_CALIBRATION = 0x01;
    static constexpr uint8_t REG_VERSION = 0x11;
    static constexpr uint8_t REG_RESISTANCE = 0x20;

    bool begin();
    bool isConnected();

    // Reads a register using the full datasheet transaction:
    // write register address, then read Data1..Data4 + CRC.
    AGS3871Error readRegister(uint8_t reg, AGS3871RegisterData &out);
    AGS3871Error readConcentration(AGS3871ConcentrationData &out);
    AGS3871Error readVersionRaw(uint32_t &version_raw);
    AGS3871Error readResistanceOhms(uint32_t &resistance_ohms);

    esp_err_t lastI2CError() const { return last_i2c_error; }

    static uint8_t crc8(const uint8_t *data, uint8_t len);

private:
    // These helpers intentionally do not install/delete the I2C driver.
    // The firmware-level Sensor wrapper owns i2c_master_init()/deinit_i2c(),
    // matching the existing I2C sensor pattern in this repository.
    AGS3871Error writeRegisterAddress(uint8_t reg);
    AGS3871Error readFiveBytes(uint8_t *buffer, uint8_t len);
    AGS3871Error readRawRegister(uint8_t reg, uint8_t *buffer);

    esp_err_t last_i2c_error = ESP_OK;
};

#endif // __AGS3871_DRIVER_H__
