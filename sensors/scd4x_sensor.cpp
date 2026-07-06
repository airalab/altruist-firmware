#include "scd4x_sensor.h"
#include "../utils.h"
#include "drivers/SCD4x_Library/src/SparkFun_SCD4x_Arduino_Library.h"
#include "../intl.h"
#include "sensor_names.h"

#define SCD4x_SENSOR_MIN_TIMEOUT 300000UL

SCD4xSensor::SCD4xSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    if (sending_timeout > SCD4x_SENSOR_MIN_TIMEOUT) {
    timeout = sending_timeout;
    } else {
    timeout = SCD4x_SENSOR_MIN_TIMEOUT;
    }
    sensor_name = SCD4X_SENSOR_NAME;
}

bool SCD4xSensor::begin() {
    debug_outln_info(F("Begin SCD4xSensor"));
    i2c_master_init();
    bool res = mySensor.begin(true, false, false, true);
    deinit_i2c();
    if (res) {
        debug_outln_info(F("SCD4x Sensor started with fetch interval (sec): "), String(timeout/1000));
    }
    last_fetch_time = millis() - timeout;
    return res;
}

void SCD4xSensor::_fetch(JsonDocument &data) {
    debug_outln_verbose(F("fetch SCD4x"));
    i2c_master_init();
    uint8_t counter = 0;
    bool is_meas = true;
    while (!mySensor.readMeasurement()) {
        counter++;
        delay(100);
        if (counter > 50) {
            is_meas = false;
            break;
        }
    }
    if (is_meas) {
        last_co2_value = mySensor.getCO2();
        last_temerature_value = mySensor.getTemperature();
        last_humidity_value = mySensor.getHumidity();

        debug_outln_verbose(F("SCD4x CO2: "), String(last_co2_value));
        debug_outln_verbose(F("SCD4x temperature: "), String(last_temerature_value));
        debug_outln_verbose(F("SCD4x humidity: "), String(last_humidity_value));

        addValueToJSON(data, F("co2"), last_co2_value, INTL_CO2, F("ppm"));
        addValueToJSON(data, F("temperature"), last_temerature_value, INTL_TEMPERATURE, F("°C"));
        addValueToJSON(data, F("humidity"), last_humidity_value, INTL_HUMIDITY, F("%"));
#if defined(ALTRUIST_BUILD_DEBUG)
        serializeJson(data, Serial);
#endif
    }
    deinit_i2c();
}
