#include "radsens_sensor.h"
#include "../utils.h"
#include "drivers/ClimateGuard_RadSens/src/CG_RadSens.h"
#include "../intl.h"
#include "sensor_names.h"

#define NOISE_READ_TIMEOUT 100

RadSensSensor::RadSensSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    timeout = sending_timeout;
    sensor_name = RADSENS_SENSOR_NAME;
}

bool RadSensSensor::begin() {
    bool res = radSens.init();
    if (res) {
        debug_outln_info(F("RadSens Sensor started with fetch interval (sec): "), String(timeout/1000));
    }
    last_fetch_time = millis() - timeout;
    return res;
}

void RadSensSensor::_fetch(JsonDocument &data) {
    I2cBusLock bus;
    if (!bus.ok()) {
        debug_outln_error(F("RadSens I2C bus lock failed"));
        return;
    }
	last_value_gc = radSens.getRadIntensyDynamic();
    debug_outln_verbose(F("radiation "), String(last_value_gc));
    addValueToJSON(data, F("radiation"), last_value_gc, INTL_RADIATION, F("µR/h"));
#if defined(ALTRUIST_BUILD_DEBUG)
    serializeJson(data, Serial);
#endif
}
