#include "sds011_sensor.h"
#include "../utils.h"
#include "../defines.h"
#include "sensor_names.h"
#include "../config_manager/config_helpers.h"

#define serialSDS (Serial1)
#define sds_SENSOR_MIN_TIMEOUT     30000UL

enum {
	SDS_REPLY_HDR = 10,
	SDS_REPLY_BODY = 8
} SDS_waiting_for;

static constexpr uint8_t kSdsRecoverAfterBadWindows = 3;

SDS011Sensor::SDS011Sensor(unsigned long sending_timeout)
	: Sensor(sending_timeout) {
	if (cfg::sds_meas_interval_ms > sds_SENSOR_MIN_TIMEOUT) {
		this->sending_timeout = cfg::sds_meas_interval_ms;
	} else {
		this->sending_timeout = sds_SENSOR_MIN_TIMEOUT;
	}
	timeout = SAMPLETIME_SDS_MS;
	sensor_name = SDS_SENSOR_NAME;
}

bool SDS011Sensor::begin() {
    serialSDS.begin(9600, SERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
    serialSDS.setTimeout((4 * 12 * 1000) / 9600);
    delay(500);
    debug_outln_info(F("Read SDS...: "), version_date());
    cmd(PmSensorCmd::ContinuousMode);
    delay(100);
    debug_outln_info(F("Stopping SDS011..."));
    is_SDS_running = cmd(PmSensorCmd::Stop);
    last_measure_time = millis() - (sending_timeout - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS));
    debug_outln_info(F("SDS011 started with fetch interval (sec): "), String(sending_timeout/1000));
    return last_value_SDS_version.length() > 0;
}

void SDS011Sensor::sdsUartRecover() {
	sds_recovery_count++;
	debug_outln_info(F("[SDS011] recover: drain UART, re-init, idle cycle"));
	debug_outln_info(F("[SDS011] recovery count: "), String(sds_recovery_count));
	while (serialSDS.available()) {
		(void)serialSDS.read();
	}
	serialSDS.flush();
#if defined(ESP32)
	serialSDS.end();
	delay(80);
#endif
	serialSDS.begin(9600, SERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
	serialSDS.setTimeout((4 * 12 * 1000) / 9600);
	delay(150);
	cmd(PmSensorCmd::ContinuousMode);
	delay(100);
	is_SDS_running = cmd(PmSensorCmd::Stop);
	SDS_waiting_for = SDS_REPLY_HDR;
	last_measure_time = millis() - (sending_timeout - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS));
}

void SDS011Sensor::_fetch(JsonDocument &data) {
	if (msSince(last_measure_time) < (sending_timeout - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {
		if (is_SDS_running) {
			is_SDS_running = cmd(PmSensorCmd::Stop);
            debug_outln_verbose(F("SDS011 stopped"));
		}
        return;
	} else {
		if (! is_SDS_running) {
			is_SDS_running = cmd(PmSensorCmd::Start);
			SDS_waiting_for = SDS_REPLY_HDR;
            debug_outln_verbose(F("SDS011 started"));
		}

		while (serialSDS.available() >= SDS_waiting_for) {
			const uint8_t constexpr hdr_measurement[2] = { 0xAA, 0xC0 };
			uint8_t data[8];

			switch (SDS_waiting_for) {
			case SDS_REPLY_HDR:
				if (serialSDS.find(hdr_measurement, sizeof(hdr_measurement)))
					SDS_waiting_for = SDS_REPLY_BODY;
				break;
			case SDS_REPLY_BODY:
				debug_outln_verbose(FPSTR(DBG_TXT_START_READING), sensor_name);
				if (serialSDS.readBytes(data, sizeof(data)) == sizeof(data) && checksum_valid(data)) {
					uint32_t pm25_serial = data[0] | (data[1] << 8);
					uint32_t pm10_serial = data[2] | (data[3] << 8);

					if (msSince(last_measure_time) > (sending_timeout - READINGTIME_SDS_MS)) {
						sds_pm10_sum += pm10_serial;
						sds_pm25_sum += pm25_serial;
						UPDATE_MIN_MAX(sds_pm10_min, sds_pm10_max, pm10_serial);
						UPDATE_MIN_MAX(sds_pm25_min, sds_pm25_max, pm25_serial);
						debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial / 10.0f));
						debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial / 10.0f));
						sds_val_count++;
					}
				}
				debug_outln_verbose(FPSTR(DBG_TXT_END_READING), sensor_name);
				SDS_waiting_for = SDS_REPLY_HDR;
				break;
			}
		}
	}
    if (msSince(last_measure_time) > sending_timeout) {
        debug_outln_verbose(F("SDS011 measure finished"));
        last_measure_time = millis();
        float last_value_SDS_P1 = -1;
        float last_value_SDS_P2 = -1;
        if (sds_val_count > 2) {
            sds_pm10_sum = sds_pm10_sum - sds_pm10_min - sds_pm10_max;
            sds_pm25_sum = sds_pm25_sum - sds_pm25_min - sds_pm25_max;
            sds_val_count = sds_val_count - 2;
        }
        if (sds_val_count > 0) {
            last_value_SDS_P1 = float(sds_pm10_sum) / (sds_val_count * 10.0f);
            last_value_SDS_P2 = float(sds_pm25_sum) / (sds_val_count * 10.0f);
            addValueToJSON(data, F("P1"), last_value_SDS_P1, "PM10", F("ppm"));
            addValueToJSON(data, F("P2"), last_value_SDS_P2, "PM2.5", F("ppm"));
            sds_bad_window_streak = 0;
            debug_outln_verbose(F("PM10: "), String(last_value_SDS_P1));
            debug_outln_verbose(F("PM2.5: "), String(last_value_SDS_P2));
            #ifdef DEV
            serializeJson(data, Serial);
            Serial.println();
            Serial.println();
            #endif
            debug_outln_verbose(F("\r\nJSON memory usage: "), String(data.memoryUsage()));
            if (sds_val_count < 3) {
                SDS_error_count++;
            }
        } else {
            SDS_error_count++;
            sds_bad_window_streak++;
            if (sds_bad_window_streak >= kSdsRecoverAfterBadWindows) {
                debug_outln_info(F("[SDS011] consecutive empty windows -> recover"));
                sdsUartRecover();
                sds_bad_window_streak = 0;
            }
        }
        sds_pm10_sum = 0;
        sds_pm25_sum = 0;
        sds_val_count = 0;
        sds_pm10_max = 0;
        sds_pm10_min = 20000;
        sds_pm25_max = 0;
        sds_pm25_min = 20000;

        if (is_SDS_running) {
            is_SDS_running = cmd(PmSensorCmd::Stop);
        }
    }
}

String SDS011Sensor::version_date() {
	if (!last_value_SDS_version.length()) {
		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(DBG_TXT_SDS011_VERSION_DATE));
		is_SDS_running = cmd(PmSensorCmd::Start);
		delay(250);
#if defined(ESP8266)
		serialSDS.perform_work();
#endif
		serialSDS.flush();
		// Query Version/Date
		rawcmd(0x07, 0x00, 0x00);
		delay(400);
		const constexpr uint8_t header_cmd_response[2] = { 0xAA, 0xC5 };
		while (serialSDS.find(header_cmd_response, sizeof(header_cmd_response))) {
			uint8_t data[8];
			unsigned r = serialSDS.readBytes(data, sizeof(data));
			if (r == sizeof(data) && data[0] == 0x07 && checksum_valid(data)) {
				char tmp[20];
				snprintf_P(tmp, sizeof(tmp), PSTR("%02d-%02d-%02d(%02x%02x)"),
					data[1], data[2], data[3], data[4], data[5]);
				last_value_SDS_version = tmp;
				break;
			}
		}
		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(DBG_TXT_SDS011_VERSION_DATE));
	}

	return last_value_SDS_version;
}

bool SDS011Sensor::checksum_valid(const uint8_t (&data)[8]) {
    uint8_t checksum_is = 0;
    for (unsigned i = 0; i < 6; ++i) {
        checksum_is += data[i];
    }
    return (data[7] == 0xAB && checksum_is == data[6]);
}

bool SDS011Sensor::rawcmd(const uint8_t cmd_head1, const uint8_t cmd_head2, const uint8_t cmd_head3) {
	constexpr uint8_t cmd_len = 19;

	uint8_t buf[cmd_len];
	buf[0] = 0xAA;
	buf[1] = 0xB4;
	buf[2] = cmd_head1;
	buf[3] = cmd_head2;
	buf[4] = cmd_head3;
	for (unsigned i = 5; i < 15; ++i) {
		buf[i] = 0x00;
	}
	buf[15] = 0xFF;
	buf[16] = 0xFF;
	buf[17] = cmd_head1 + cmd_head2 + cmd_head3 - 2;
	buf[18] = 0xAB;
	const size_t written = serialSDS.write(buf, cmd_len);
	if (written != cmd_len) {
		debug_outln_error(F("[SDS011] command write failed"));
		return false;
	}
	return true;
}

bool SDS011Sensor::cmd(PmSensorCmd cmd) {
	bool write_ok = true;
	switch (cmd) {
	case PmSensorCmd::Start:
		write_ok = rawcmd(0x06, 0x01, 0x01);
		break;
	case PmSensorCmd::Stop:
		write_ok = rawcmd(0x06, 0x01, 0x00);
		break;
	case PmSensorCmd::ContinuousMode:
		// TODO: Check mode first before (re-)setting it
		write_ok = rawcmd(0x08, 0x01, 0x00);
		write_ok = rawcmd(0x02, 0x01, 0x00) && write_ok;
		break;
	}

	return write_ok && cmd != PmSensorCmd::Stop;
}
