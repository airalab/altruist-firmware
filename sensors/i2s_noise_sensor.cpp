#include "i2s_noise_sensor.h"
#include "../utils.h"
#include "drivers/i2s_noise/i2s_noise.h"
#include "../intl.h"
#include "sensor_names.h"

#define NOISE_READ_TIMEOUT 100

I2SNoiseSensor::I2SNoiseSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    timeout = NOISE_READ_TIMEOUT;
    sensor_name = I2S_NOISE_SENSOR_NAME;
}

bool I2SNoiseSensor::begin() {
    float db_mean = 0;
    fetchSensorI2sSound(&last_value_DBMETER, &db_mean);
    last_send_time = millis();
    debug_outln_info(F("I2S Noise Sensor started with fetch interval (sec): "), String(sending_timeout/1000));
    return db_mean > 0;
}

void I2SNoiseSensor::_fetch(JsonDocument &data) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), sensor_name);
    float db_mean;
	if (is_SDS_running) {
		debug_outln_verbose(F("Don't measure noise: SDS is running"));
        return;
	} else {
		fetchSensorI2sSound(&last_value_DBMETER, &db_mean);
		if (last_value_DBMETER > last_value_DBMETER_max) {
			last_value_DBMETER_max = last_value_DBMETER;
		}
		last_value_DBMETER_sum += db_mean;
		last_value_DBMETER_count++;
		last_value_DBMETER_mean = (float)last_value_DBMETER_sum / (float)last_value_DBMETER_count;
	}
    // debug_outln_info(F("Noise max: "), last_value_DBMETER_max);
    // debug_outln_info(F("Noise mean: "), last_value_DBMETER_mean);
    // String noise_mean(last_value_DBMETER_mean, 0);
    uint8_t last_value_DBMETER_mean_round = (uint8_t)last_value_DBMETER_mean;
    addValueToJSON(data, F("noiseMax"), last_value_DBMETER_max, INTL_NOISE_MAX, F("dB"));
    addValueToJSON(data, F("noiseAvg"), last_value_DBMETER_mean_round, INTL_NOISE_MEAN, F("dB"));

    debug_outln_verbose(F("Noise sum: "), String(last_value_DBMETER_sum));
    debug_outln_verbose(F("Noise count: "), String(last_value_DBMETER_count));
    debug_outln_verbose(F("I2S noise: "), String(db_mean));
    debug_outln_verbose(F("I2S noiseMax: "), String(last_value_DBMETER_max));
    debug_outln_verbose(F("I2S noiseAvg: "), String(last_value_DBMETER_mean));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), sensor_name);
    if (msSince(last_send_time) > sending_timeout) {
        debug_outln_verbose(FPSTR(DBG_TXT_SEP));
        debug_outln_verbose(F("I2S noise: "), String(db_mean));
        debug_outln_verbose(F("I2S noiseMax: "), String(last_value_DBMETER_max));
        debug_outln_verbose(F("I2S noiseAvg: "), String(last_value_DBMETER_mean));
        #if defined(ALTRUIST_BUILD_DEBUG)
        serializeJson(data, Serial);
        Serial.println();
        Serial.println();
        #endif
        debug_outln_verbose(F("\r\nJSON memory usage: "), String(data.memoryUsage()));
        last_send_time = millis();
        reset_values();
        debug_outln_verbose(F("Reset noise values"));
    }
}

void I2SNoiseSensor::reset_values() {
    last_value_DBMETER_max = 0;
    last_value_DBMETER_mean = 0.0;
    last_value_DBMETER_count = 0.0;
    last_value_DBMETER_sum = 0;
}
