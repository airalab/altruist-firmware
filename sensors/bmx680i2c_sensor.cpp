#include "bmx680i2c_sensor.h"
#include "../intl.h"
#include "../config_manager/config_helpers.h"
#include "sensor_names.h"

#define BME680_SENSOR_MIN_TIMEOUT 300000

BME680Sensor::BME680Sensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) 
    , bme680(I2C_NUM_0, BME68X_DEFAULT_ADDRESS) 
    {
    if (sending_timeout > BME680_SENSOR_MIN_TIMEOUT) {
        timeout = sending_timeout;
    } else {
        timeout = BME680_SENSOR_MIN_TIMEOUT;
    }
    sensor_name = BME680_SENSOR_NAME;
}

bool BME680Sensor::begin() {
    debug_outln_info(F("Begin BME680Sensor"));
    i2c_master_init();
    bool res = bme680.begin(); 
    deinit_i2c();
    if (res) {
        debug_outln_info(F("BME680 Sensor started with fetch interval (sec): "), String(timeout / 1000));
    }
    last_fetch_time = millis() - timeout;
    return res;
}

void BME680Sensor::_fetch(JsonDocument &data) {
    debug_outln_info(F("fetch BME680"));

    i2c_master_init();

    if (bme680.performReading()) {
        last_temperature_value = bme680.temperature;    // °C
        last_pressure_value = bme680.pressure / 100.0;  // hPa
        last_humidity_value = bme680.humidity;          // %
        // last_gas_resistance_value = bme680.gas_resistance; // Ом (Омми)

        debug_outln_info(F("BME680 temperature: "), String(last_temperature_value));
        debug_outln_info(F("BME680 pressure: "), String(last_pressure_value));
        debug_outln_info(F("BME680 humidity: "), String(last_humidity_value));
        // debug_outln_info(F("BME680 gas resistance: "), String(last_gas_resistance_value));

        addValueToJSON(data, F("temperature"), last_temperature_value, INTL_TEMPERATURE, F("°C"));
        addValueToJSON(data, F("pressure"), last_pressure_value, INTL_PRESSURE, F("hPa"));
        addValueToJSON(data, F("humidity"), last_humidity_value, INTL_HUMIDITY, F("%"));

        serializeJson(data, Serial);
    } else {
        debug_outln_error(F("BME680 reading failed"));
    }

    deinit_i2c();
}
