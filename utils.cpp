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

#include <WString.h>

#include "./intl.h"
#include "./utils.h"
#include "./defines.h"
//#include "./ext_def.h"
#include <SPIFFS.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_attr.h>

#define RESTART_REASON_MAGIC 0xA5C6F012
RTC_NOINIT_ATTR static uint32_t rtc_restart_magic;
RTC_NOINIT_ATTR static uint32_t rtc_restart_reason;

void set_restart_reason(CustomRestartReason reason) {
	rtc_restart_magic = RESTART_REASON_MAGIC;
	rtc_restart_reason = static_cast<uint32_t>(reason);
}
#include <time.h>

#ifdef ESP32
#if defined(CONFIG_IDF_TARGET_ESP32C6)
#include "driver/temperature_sensor.h"
static temperature_sensor_handle_t temp_sensor_handle = NULL;
static bool temp_sensor_initialized = false;
#endif
#endif

String get_chipid() {
	static String cached_chipid = "";  // Static variable to cache the ID
	
	if (cached_chipid.length() == 0) {  // Only get it once
		WiFiMode_t previousMode = WiFi.getMode();  // Save current mode
		WiFi.mode(WIFI_STA);  // Initialize WiFi to get MAC
		String mac = WiFi.macAddress();
		mac.replace(":", "");  // "AA:BB:CC:DD:EE:FF" -> "AABBCCDDEEFF"
		cached_chipid = mac;
		
		// Restore previous mode only if it wasn't OFF
		if (previousMode != WIFI_OFF) {
			WiFi.mode(previousMode);
		}
		// If it was OFF, leave it in STA mode for the wifi manager to handle
	}
	
	return cached_chipid;
}

String tmpl(const __FlashStringHelper* patt, const String& value) {
	String s = patt;
	s.replace("{v}", value);
	return s;
}

const char* get_reset_reason_text() {
    static const char* cached = nullptr;
    if (cached) return cached;

    esp_reset_reason_t reason = esp_reset_reason();

    // Check custom restart reason stored in RTC memory (survives software resets)
    if (reason == ESP_RST_SW && rtc_restart_magic == RESTART_REASON_MAGIC) {
        uint32_t custom = rtc_restart_reason;
        rtc_restart_magic = 0;
        switch (custom) {
            case RESTART_REASON_OTA:    cached = "OTA firmware update"; return cached;
            case RESTART_REASON_CONFIG: cached = "Configuration saved"; return cached;
            case RESTART_REASON_USER:   cached = "User restart (web)"; return cached;
        }
    }
    rtc_restart_magic = 0;

    switch (reason) {
        case ESP_RST_POWERON:   cached = "Power-on reset"; break;
        case ESP_RST_EXT:       cached = "External reset"; break;
        case ESP_RST_SW:        cached = "Software reset"; break;
        case ESP_RST_PANIC:
#if defined(ALTRUIST_BUILD_DEBUG)
            cached = "Panic (unhandled exception)";
#else
            cached = "Unexpected restart (recovered)";
#endif
            break;
        case ESP_RST_INT_WDT:   cached = "Interrupt watchdog timeout"; break;
        case ESP_RST_TASK_WDT:  cached = "Task watchdog timeout"; break;
        case ESP_RST_WDT:       cached = "Other watchdog reset"; break;
        case ESP_RST_DEEPSLEEP: cached = "Wake from deep sleep"; break;
        case ESP_RST_BROWNOUT:  cached = "Brownout (voltage too low)"; break;
        case ESP_RST_SDIO:      cached = "Reset over SDIO"; break;
        case (esp_reset_reason_t)11: cached = "USB reset (flash/boot)"; break;
        case (esp_reset_reason_t)12: cached = "JTAG reset"; break;
        case (esp_reset_reason_t)13: cached = "eFuse error"; break;
        case (esp_reset_reason_t)14: cached = "Power glitch"; break;
        case (esp_reset_reason_t)15: cached = "CPU lock-up"; break;
        default: {
            static char unknown_buf[24];
            snprintf(unknown_buf, sizeof(unknown_buf), "Unknown (%d)", (int)reason);
            cached = unknown_buf;
            break;
        }
    }
    return cached;
}

String delayToString(unsigned time_ms) {

	char buf[64];
	String s;

	if (time_ms > 2 * 1000 * 60 * 60 * 24) {
		sprintf_P(buf, PSTR("%d days, "), time_ms / (1000 * 60 * 60 * 24));
		s += buf;
		time_ms %= 1000 * 60 * 60 * 24;
	}

	if (time_ms > 2 * 1000 * 60 * 60) {
		sprintf_P(buf, PSTR("%d hours, "), time_ms / (1000 * 60 * 60));
		s += buf;
		time_ms %= 1000 * 60 * 60;
	}

	if (time_ms > 2 * 1000 * 60) {
		sprintf_P(buf, PSTR("%d min, "), time_ms / (1000 * 60));
		s += buf;
		time_ms %= 1000 * 60;
	}

	if (time_ms > 2 * 1000) {
		sprintf_P(buf, PSTR("%ds, "), time_ms / 1000);
		s += buf;
	}

	if (s.length() > 2) {
		s = s.substring(0, s.length() - 2);
	}

	return s;
}

void sensor_restart() {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"

		SPIFFS.end();

#pragma GCC diagnostic pop

		serialSDS.end();
		// digitalWrite(PM_RESTART, LOW);
		// debug_outln_info(F("Restart."));
		delay(5000);
		ESP.restart();
		// should not be reached
		while(true) { yield(); }
}


float readCorrectionOffset(const char* correction) {
	char* pEnd = nullptr;
	// Avoiding atof() here as this adds a lot (~ 9kb) of code size
	float r = float(strtol(correction, &pEnd, 10));
	if (pEnd && pEnd[0] == '.' && pEnd[1] >= '0' && pEnd[1] <= '9') {
		r += (correction[0] == '-' ? -1.0f : 1.0f) * ((pEnd[1] - '0') / 10.0f);
	}
	return r;
}

/*****************************************************************
 * Debug output                                                  *
 *****************************************************************/

LoggingSerial Debug;

LoggingSerial::LoggingSerial()
    : HardwareSerial(0)
{
	m_buffer = xQueueCreate(LARGE_STR, sizeof(uint8_t));
}

size_t LoggingSerial::write(uint8_t c)
{
	xQueueSendToBack(m_buffer, ( void * ) &c, ( TickType_t ) 1);
	return HardwareSerial::write(c);
}

size_t LoggingSerial::write(const uint8_t *buffer, size_t size)
{
	for(int i = 0; i < size; i++) {
		xQueueSendToBack(m_buffer, ( void * ) &buffer[i], ( TickType_t ) 1);
	}
	return HardwareSerial::write(buffer, size);
}

String LoggingSerial::popLines()
{
	String r;
	uint8_t c;
	while (xQueueReceive(m_buffer, &(c ), (TickType_t) 1 )) {
		r += (char) c;

		if (c == '\n' && r.length() > 10)
			break;
	}
	return r;
}

unsigned int effectiveRuntimeLogLevel() {
	return max(static_cast<unsigned int>(ALTRUIST_FORCE_LOG_LEVEL), cfg::debug);
}

#define debug_level_check(level) { if (level > effectiveRuntimeLogLevel()) return; }

void debug_out(const String& text, unsigned int level) {
	debug_level_check(level); Debug.print(text); Serial.print(text);
}

void debug_out(const __FlashStringHelper* text, unsigned int level) {
	debug_level_check(level); Debug.print(text); Serial.print(text);
}

void debug_outln(const String& text, unsigned int level) {
	debug_level_check(level); Debug.println(text); Serial.println(text);
}

void debug_outln_info(const String& text) {
	String tagged = "[INFO] " + text;
	String dated_text = "[" + String(millis()) + "] " + tagged; 
	debug_level_check(DEBUG_MIN_INFO);
	Debug.println(tagged);
	Serial.println(dated_text);
}

void debug_outln_verbose(const String& text) {
	debug_level_check(DEBUG_MED_INFO); Debug.println(text); Serial.println(text);
}

void debug_outln_error(const __FlashStringHelper* text) {
	debug_level_check(DEBUG_ERROR);
	String tagged = "[ERROR] ";
	tagged += String(text);
	Debug.println(tagged);
	Serial.println(tagged);
}

void debug_outln_info(const __FlashStringHelper* text) {
	String tagged = "[INFO] ";
	tagged += String(text);
	String dated_text = "[" + String(millis()) + "] " + tagged; 
	debug_level_check(DEBUG_MIN_INFO); Debug.println(text); Serial.println(dated_text);
}

void debug_outln_verbose(const __FlashStringHelper* text) {
	debug_level_check(DEBUG_MED_INFO);
	String tagged = "[DEBUG] ";
	tagged += String(text);
	Debug.println(tagged);
	Serial.println(tagged);
}

void debug_outln_info(const __FlashStringHelper* text, const String& option) {
	String tagged = "[INFO] ";
	tagged += String(text);
	String dated_text = "[" + String(millis()) + "] " + tagged; 
	debug_level_check(DEBUG_MIN_INFO);
	Debug.print(tagged);
	Debug.println(": " + option);
	Serial.print(dated_text);
	Serial.println(": " + option);
}

void debug_outln_info(const __FlashStringHelper* text, float value) {
	debug_outln_info(text, String(value));
}

void debug_outln_verbose(const __FlashStringHelper* text, const String& option) {
	debug_level_check(DEBUG_MED_INFO);
	String tagged = "[DEBUG] ";
	tagged += String(text);
	Debug.print(tagged);
	Debug.println(": " + option);
	Serial.print(tagged);
	Serial.println(": " + option);
}

void debug_outln_info_bool(const __FlashStringHelper* text, const bool option) {
	debug_level_check(DEBUG_MIN_INFO);
	String tagged = "[INFO] ";
	tagged += String(text);
	Debug.print(tagged);
	Debug.println(": " + String(option));
	Serial.print(tagged);
	Serial.println(": " + String(option));
}

#undef debug_level_check

/*****************************************************************
 * helper to see if a given string is numeric                    *
 *****************************************************************/
bool isNumeric(const String& str) {
	size_t stringLength = str.length();

	if (stringLength == 0) {
		return false;
	}

	bool seenDecimal = false;

	for (size_t i = 0; i < stringLength; ++i) {
		if (i == 0 && str.charAt(0) == '-') {
			continue;
		}

		if (isDigit(str.charAt(i))) {
			continue;
		}

		if (str.charAt(i) == '.') {
			if (seenDecimal) {
				return false;
			}
			seenDecimal = true;
			continue;
		}
		return false;
	}
	return true;
}

// Global metrics instance
metrics_t system_metrics;

// Initialize metrics (load boot counter from NVS, increment it)
void initMetrics() {
	Preferences preferences;
	preferences.begin("metrics", false);
	system_metrics.boot_counter = preferences.getULong("boot_cnt", 0);
	system_metrics.boot_counter++; // Increment on boot
	preferences.putULong("boot_cnt", system_metrics.boot_counter);
	preferences.end();
	
	system_metrics.uptime_sec = 0;
	system_metrics.tx_counter = 0;
	system_metrics.last_telemetry_timestamp = 0;
	system_metrics.err_wifi_reconnects = 0;
	system_metrics.err_sensor = 0;
	system_metrics.err_sd_write = 0;
	system_metrics.has_error = false;
}

// Update metrics (uptime, temperature, etc.)
void updateMetrics() {
	system_metrics.uptime_sec = millis() / 1000;
	system_metrics.esp_temperature = getESPTemperature();
	
	// Check if we have errors
	system_metrics.has_error = (system_metrics.err_wifi_reconnects > 0 || 
	                           system_metrics.err_sensor > 0 || 
	                           system_metrics.err_sd_write > 0);
}

// Initialize ESP32-C6 temperature sensor (call once at startup)
void initESPTemperatureSensor() {
	#ifdef ESP32
		#if defined(CONFIG_IDF_TARGET_ESP32C6)
			if (temp_sensor_initialized) {
				return; // Already initialized
			}
			
			temperature_sensor_config_t temp_sensor_config = {
				.range_min = -10,
				.range_max = 80,
			};
			
			esp_err_t ret = temperature_sensor_install(&temp_sensor_config, &temp_sensor_handle);
			if (ret != ESP_OK) {
				Serial.printf("[URBAN][Temp] Failed to install temperature sensor: %s\n", esp_err_to_name(ret));
				return;
			}
			
			ret = temperature_sensor_enable(temp_sensor_handle);
			if (ret != ESP_OK) {
				Serial.printf("[URBAN][Temp] Failed to enable temperature sensor: %s\n", esp_err_to_name(ret));
				temperature_sensor_uninstall(temp_sensor_handle);
				temp_sensor_handle = NULL;
				return;
			}
			
			temp_sensor_initialized = true;
			Serial.println(F("[URBAN][Temp] ESP32-C6 temperature sensor initialized"));
		#endif
	#endif
}

// Get ESP temperature (ESP32-C6 internal temperature sensor)
float getESPTemperature() {
	#ifdef ESP32
		#if defined(CONFIG_IDF_TARGET_ESP32C6)
			if (!temp_sensor_initialized || temp_sensor_handle == NULL) {
				return 0.0; // Sensor not initialized
			}
			
			float temperature = 0.0;
			esp_err_t ret = temperature_sensor_get_celsius(temp_sensor_handle, &temperature);
			if (ret != ESP_OK) {
				// If reading fails, return last known value or 0
				return 0.0;
			}
			return temperature;
		#else
			// Other ESP32 variants - no built-in temperature sensor
			return 0.0;
		#endif
	#else
		// Fallback: return 0 if not available
		return 0.0;
	#endif
}

// Increment error counters
void incrementWiFiReconnectError() {
	system_metrics.err_wifi_reconnects++;
}

void incrementSensorError() {
	system_metrics.err_sensor++;
}

void incrementSDWriteError() {
	system_metrics.err_sd_write++;
}

void incrementTXCounter() {
	system_metrics.tx_counter++;
	system_metrics.last_telemetry_timestamp = time(nullptr);
}

// Log a stable, machine-readable health snapshot to UART.
void logMetrics() {
#if defined(ALTRUIST_HEALTH_TELEMETRY)
	updateMetrics();
	const bool wifi_connected = WiFi.status() == WL_CONNECTED;
	const int rssi = wifi_connected ? WiFi.RSSI() : 0;
	const unsigned long error_count =
		system_metrics.err_wifi_reconnects +
		system_metrics.err_sensor +
		system_metrics.err_sd_write;
#if defined(ESP32)
	const uint32_t free_heap = ESP.getFreeHeap();
#else
	const uint32_t free_heap = 0;
#endif

	Serial.print(F("[HEALTH] uptime="));
	Serial.print(system_metrics.uptime_sec);
	Serial.print(F(" boot="));
	Serial.print(system_metrics.boot_counter);
	Serial.print(F(" heap="));
	Serial.print(free_heap);
	Serial.print(F(" rssi="));
	Serial.print(rssi);
	Serial.print(F(" tx="));
	Serial.print(system_metrics.tx_counter);
	Serial.print(F(" errors="));
	Serial.print(error_count);
	Serial.print(F(" wifi="));
	Serial.print(wifi_connected ? 1 : 0);
	Serial.print(F(" wifi_errors="));
	Serial.print(system_metrics.err_wifi_reconnects);
	Serial.print(F(" sensor_errors="));
	Serial.print(system_metrics.err_sensor);
	Serial.print(F(" sd_errors="));
	Serial.println(system_metrics.err_sd_write);
#endif
}
