/************************************************************************
 *                                                                      *
 *    airRohr firmware                                                  *
 *    Copyright (C) 2016-2020  Code for Stuttgart a.o.                  *
 *    Copyright (C) 2019-2020  Dirk Mueller                             *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 *                                                                      *
 ************************************************************************
 */


#ifndef utils_h
#define utils_h

#include <WString.h>
#include <map>
#include <vector>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HardwareSerial.h>
#include "sha/sha_parallel_engine.h"
#include <freertos/queue.h>

const char DBG_TXT_TEMPERATURE[] PROGMEM = "Temperature (°C): ";
const char DBG_TXT_DECIBEL[] PROGMEM = "Noise Level (DB): ";
const char DBG_TXT_HUMIDITY[] PROGMEM = "Humidity (%): ";
const char DBG_TXT_PRESSURE[] PROGMEM = "Pressure (hPa): ";
const char DBG_TXT_START_READING[] PROGMEM = "R/ ";
const char DBG_TXT_END_READING[] PROGMEM = "/R ";
const char DBG_TXT_CHECKSUM_IS[] PROGMEM = "Checksum is: ";
const char DBG_TXT_CHECKSUM_SHOULD[] PROGMEM = "Checksum should: ";
const char DBG_TXT_DATA_READ_FAILED[] PROGMEM = "Data read failed";
const char DBG_TXT_UPDATE[] PROGMEM = "[update] ";
const char DBG_TXT_UPDATE_FAILED[] PROGMEM = "Update failed.";
const char DBG_TXT_UPDATE_NO_UPDATE[] PROGMEM = "No update.";
const char DBG_TXT_SENDING_TO[] PROGMEM = "## Sending to ";
const char DBG_TXT_SDS011_VERSION_DATE[] PROGMEM = "SDS011 version date";
const char DBG_TXT_CONNECTING_TO[] PROGMEM = "Connecting to ";
const char DBG_TXT_FOUND[] PROGMEM = " ... found";
const char DBG_TXT_NOT_FOUND[] PROGMEM = " ... not found";
const char DBG_TXT_SEP[] PROGMEM = "----";

const char JSON_DATA_VALUES[] PROGMEM = "values";

constexpr unsigned SMALL_STR = 64-1;
constexpr unsigned MED_STR = 256-1;
constexpr unsigned LARGE_STR = 512-1;
constexpr unsigned XLARGE_STR = 1024-1;

#define msSince(timestamp_before) (millis() - (timestamp_before))

#define RESERVE_STRING(name, size) String name((const char*)nullptr); name.reserve(size)

#define UPDATE_MIN(MIN, SAMPLE) if (SAMPLE < MIN) { MIN = SAMPLE; }
#define UPDATE_MAX(MAX, SAMPLE) if (SAMPLE > MAX) { MAX = SAMPLE; }
#define UPDATE_MIN_MAX(MIN, MAX, SAMPLE) { UPDATE_MIN(MIN, SAMPLE); UPDATE_MAX(MAX, SAMPLE); }

struct api_status_t {
	bool is_ok = true;
	unsigned long count_sends = 0;
	time_t last_send_time;
};

struct device_status_t {
	unsigned long last_update_attempt;
	unsigned long time_point_device_start_ms;
	int last_update_returncode;
	unsigned long count_sends = 0;
	std::map<std::string, api_status_t> apis_status;
	std::vector<std::string> sensor_names;
};

String get_chipid();
String tmpl(const __FlashStringHelper* patt, const String& value);
const char* get_reset_reason_text();

String wlan_ssid_to_table_row(const String& ssid, const String& encryption, int32_t rssi);
String delayToString(unsigned time_ms);

void sensor_restart();


extern float readCorrectionOffset(const char* correction);

namespace cfg {
	extern unsigned debug;
}

#define serialSDS (Serial1)


/*****************************************************************
 * Debug output                                                  *
 *****************************************************************/

class LoggingSerial : public HardwareSerial {

public:
	LoggingSerial();
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
	String popLines();

private:
	QueueHandle_t m_buffer;
};

extern class LoggingSerial Debug;

extern void debug_out(const String& text, unsigned int level);
extern void debug_out(const __FlashStringHelper* text, unsigned int level);
extern void debug_outln(const String& text, unsigned int level);
extern void debug_outln_info(const String& text);
extern void debug_outln_verbose(const String& text);
extern void debug_outln_error(const __FlashStringHelper* text);
extern void debug_outln_info(const __FlashStringHelper* text);
extern void debug_outln_verbose(const __FlashStringHelper* text);
extern void debug_outln_info(const __FlashStringHelper* text, const String& option);
extern void debug_outln_info(const __FlashStringHelper* text, float value);
extern void debug_outln_verbose(const __FlashStringHelper* text, const String& option);
extern void debug_outln_info_bool(const __FlashStringHelper* text, const bool option);


extern bool isNumeric(const String& str);

extern const __FlashStringHelper* loggerDescription(unsigned i);

#endif
