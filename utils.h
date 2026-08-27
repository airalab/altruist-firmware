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
	unsigned long count_sends_success = 0;
	time_t last_send_time = 0;
};

struct device_status_t {
	unsigned long last_update_attempt;
	unsigned long time_point_device_start_ms;
	int last_update_returncode;
	bool sd_card_connected = false;
	bool ota_in_progress = false;  // When true, display shows "Updating firmware" screen
	bool ota_failed = false;       // When true, display shows "Update failed" screen before returning
	bool ota_success = false;      // When true, display shows "Update successful" screen before restart
	int ota_progress_percent = -1;  // 0-100 during download, -1 when not applicable
	volatile bool ota_update_requested = false;  // Set by webserver to trigger manual OTA install
	/** UI result of last manual OTA check (hub / /ota). */
	enum OtaCheckUi : uint8_t {
		OtaCheckUi_None = 0,
		OtaCheckUi_UpToDate = 1,
		OtaCheckUi_Available = 2,
		OtaCheckUi_Failed = 3,
		OtaCheckUi_InstallQueued = 4,
	};
	OtaCheckUi ota_check_ui = OtaCheckUi_None;
	char ota_remote_version[48] = "";
	String ip_address;
	std::map<std::string, api_status_t> apis_status;
	std::vector<std::string> sensor_names;
};

// UART metrics structure for status logging
struct metrics_t {
	unsigned long boot_counter = 0;
	unsigned long uptime_sec = 0;
	int reset_reason_code = 0;
	char reset_reason[32] = "unknown";
	bool crash_context_valid = false;
	unsigned long prev_uptime_sec = 0;
	unsigned long prev_free_heap = 0;
	unsigned int last_section_id = 0;
	char last_section[24] = "unknown";
	unsigned long tx_counter = 0;
	time_t last_telemetry_timestamp = 0;
	unsigned long err_wifi_reconnects = 0;
	unsigned long err_sensor = 0;
	unsigned long err_sd_write = 0;
	float esp_temperature = 0.0;
	bool has_error = false;
};

String get_chipid();
String tmpl(const __FlashStringHelper* patt, const String& value);
enum CustomRestartReason : uint32_t {
	RESTART_REASON_NONE = 0,
	RESTART_REASON_OTA = 1,
	RESTART_REASON_CONFIG = 2,
	RESTART_REASON_USER = 3,
	RESTART_REASON_WATCHDOG = 4,
	RESTART_REASON_WIFI = 5,
};

void set_restart_reason(CustomRestartReason reason);
const char* get_reset_reason_text();

/** Feed the main-loop stall watchdog during long HTTP or e-ink work. */
void markMainLoopAlive();

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
	void beginStructuredOutput(unsigned long baud, int8_t rx_pin, int8_t tx_pin);
	bool writeStructuredLine(const String& line);
	String popLines();

private:
	QueueHandle_t m_buffer;
	SemaphoreHandle_t m_write_mutex;
	bool m_structured_output = false;
};

extern class LoggingSerial Debug;

extern unsigned int effectiveRuntimeLogLevel();
/** Mute USB CDC logs while Improv Serial owns the port (webflasher). */
extern void debugSetUsbQuiet(bool quiet);
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
extern void logSubsystemEvent(const __FlashStringHelper* level, const __FlashStringHelper* subsystem, const __FlashStringHelper* reason);
extern void logSubsystemEvent(const __FlashStringHelper* level, const __FlashStringHelper* subsystem, const __FlashStringHelper* reason, const String& details);
extern void logSubsystemError(const __FlashStringHelper* subsystem, const __FlashStringHelper* reason);
extern void logSubsystemError(const __FlashStringHelper* subsystem, const __FlashStringHelper* reason, const String& details);


extern bool isNumeric(const String& str);

extern const __FlashStringHelper* loggerDescription(unsigned i);

// UART metrics logging functions
extern metrics_t system_metrics;
void initMetrics();
void updateMetrics();
void logMetrics();
void incrementWiFiReconnectError();
void incrementSensorError();
void incrementSDWriteError();
void incrementTXCounter();
void initESPTemperatureSensor();
float getESPTemperature();

#endif
