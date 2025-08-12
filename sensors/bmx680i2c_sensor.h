#ifndef __BMX680_SENSOR_H__
#define __BMX680_SENSOR_H__

#include "Arduino.h"
#include "sensor.h"

#include "driver/i2c.h"
#include "drivers/i2c.h"
#include "drivers/bme680/bme680.h"

#define BME680_ADDR_PRIMARY 0x76
#define BME680_ADDR_SECONDARY 0x77


class BME680Sensor : public Sensor {
public:
    explicit BME680Sensor(unsigned long sending_timeout = 300000UL);
    ~BME680Sensor();
    bool begin();

protected:
    void _fetch(JsonDocument &data) override;

private:
    Adafruit_BME680 *bme680 = nullptr;
    uint8_t sensor_address = 0;

    // Последние считанные значения
    float last_temperature_value = 0.0f;
    float last_pressure_value = 0.0f;
    float last_humidity_value = 0.0f;
    uint32_t last_gas_resistance_value = 0;

    // Минимальный таймаут, например 5 минут (300000 мс)
    static constexpr unsigned long BME680_SENSOR_MIN_TIMEOUT = 300000UL;
};

#endif //__BMX680_SENSOR_H__
