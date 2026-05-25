/************************************************************************
 *                                                                      *
 *  This source code needs to be compiled for the board                 *
 *  NodeMCU 1.0 (ESP-12E Module)                                        *
 *                                                                      *
 ************************************************************************
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
 * OK LAB Particulate Matter Sensor                                     *
 *      - nodemcu-LoLin board                                           *
 *      - Nova SDS0111                                                  *
 *  http://inovafitness.com/en/Laser-PM2-5-Sensor-SDS011-35.html        *
 *                                                                      *
 * Wiring Instruction see included Readme.md                            *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Alternative                                                          *
 *      - nodemcu-LoLin board                                           *
 *                                                                      *
 * Wiring Instruction:                                                  *
 *      Pin 2 of dust sensor PM2.5 -> Digital 6 (PWM)                   *
 *      Pin 3 of dust sensor       -> +5V                               *
 *      Pin 4 of dust sensor PM1   -> Digital 3 (PMW)                   *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Please check Readme.md for other sensors and hardware                *
 *                                                                      *
 ************************************************************************
 *
 * latest mit lib 2.6.1
 * DATA:    [====      ]  40.7% (used 33316 bytes from 81920 bytes)
 * PROGRAM: [=====     ]  49.3% (used 514788 bytes from 1044464 bytes)

 * latest mit lib 2.5.2
 * DATA:    [====      ]  39.4% (used 32304 bytes from 81920 bytes)
 * PROGRAM: [=====     ]  48.3% (used 504812 bytes from 1044464 bytes)
 *
 ************************************************************************/
#include <WString.h>
#include <pgmspace.h>

/*****************************************************************
 * Includes                                                      *
 *****************************************************************/

// ESP32 system includes for reset reason
#include <esp_system.h>
#include <Preferences.h>
#if defined(ESP32)
// Run some lwIP operations on the TCPIP thread (ESP32-C6 asserts otherwise).
#include <lwip/tcpip.h>
#include <lwip/apps/sntp.h>
#include <esp_netif.h>
#ifdef CONFIG_LWIP_TCPIP_CORE_LOCKING
// Needed for sys_thread_tcpip() + LWIP_CORE_LOCK_QUERY_HOLDER (Arduino core uses it too).
#include <lwip/priv/tcpip_priv.h>
#endif
#endif

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>

#include "./intl.h"

#include "./utils.h"
#include "defines.h"
//#include "ext_def.h"
#include "webserver/html-content.h"
#include <Robonomics.h>
#include "sensors/sensor_factory.h"
#include "apis/apis.h"
#include "config_manager/config_helpers.h"
#include "wifi_manager.h"
#include "webserver/webserver.h"
#include "OTA_Update.h"
#include "sd_card/sd_card.h"
#include "buttons/button_manager.h"
#if defined(ALTRUIST_INSIDE)
#include "display/display_manager.h"
#include "leds/leds_controller_insight.h"
#endif
#if defined(ALTRUIST_URBAN)
#include "leds/leds_controller_urban.h"
#endif

String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);

// Needed for Arduino .ino auto-generated prototypes in non-INSIDE builds.
struct analytics_screen_values_t;

static constexpr size_t SENSORS_JSON_CAPACITY = 4096;
SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
DynamicJsonDocument sensors_data(SENSORS_JSON_CAPACITY);
device_status_t deviceStatus;
#if defined(USE_SD_CARD)
SDCard sdCardLogger;
#endif

#if defined(ALTRUIST_INSIDE)
DisplayManager displayManager(sensors_data, deviceStatus, mutex);
#endif
ButtonManager button_manager;

button_pressed_t btn_press;

#if defined(ALTRUIST_URBAN)
LedControllerUrban leds_controller_urban;
static volatile bool urban_api_status_ok = true;
static volatile bool urban_datalog_sending = false;
static volatile int urban_datalog_result = 0;
static volatile unsigned long urban_datalog_result_until_ms = 0;
#endif
#if defined(ALTRUIST_INSIDE)
LedControllerInsight leds_controller_insight(sensors_data, mutex);
#endif

SensorWebServer webserver(sensors_data, deviceStatus, mutex);

/*****************************************************************
 * Crash Instrumentation (simplified)                            *
 *****************************************************************/
#define CRASH_SECTION_IDLE              0
#define CRASH_SECTION_FETCH_SENSORS     1
#define CRASH_SECTION_ROBONOMICS_DATALOG 2
#define CRASH_SECTION_ROBONOMICS_HTTP_MAP 3
#define CRASH_SECTION_CUSTOM_HTTP       4
#define CRASH_SECTION_DISPLAY_UPDATE    5
#define CRASH_SECTION_LED_UPDATE        6
#define CRASH_SECTION_WIFI_RECONNECT    7
#define CRASH_SECTION_SD_WRITE          8

// Struct must be defined before functions that use it (for Arduino auto-prototyping)
struct CrashContextData {
	bool valid;
	uint8_t section;
	uint32_t uptime_sec;
	uint32_t free_heap;
};

static uint8_t crash_last_section = 0;

void markCrashSection(uint8_t section) {
	crash_last_section = section;
}

const char* getCrashSectionName(uint8_t section) {
	switch (section) {
		case CRASH_SECTION_IDLE:              return "Idle/MainLoop";
		case CRASH_SECTION_FETCH_SENSORS:     return "FetchSensors";
		case CRASH_SECTION_ROBONOMICS_DATALOG: return "RobonomicsDatalog";
		case CRASH_SECTION_ROBONOMICS_HTTP_MAP: return "RobonomicsHTTPMap";
		case CRASH_SECTION_CUSTOM_HTTP:       return "CustomHTTP";
		case CRASH_SECTION_DISPLAY_UPDATE:    return "DisplayUpdate";
		case CRASH_SECTION_LED_UPDATE:        return "LEDUpdate";
		case CRASH_SECTION_WIFI_RECONNECT:    return "WiFiReconnect";
		case CRASH_SECTION_SD_WRITE:          return "SDWrite";
		default:                              return "Unknown";
	}
}

void saveCrashContext() {
	Preferences prefs;
	prefs.begin("crash", false);
	prefs.putUChar("section", crash_last_section);
	prefs.putULong("uptime", millis() / 1000);
	prefs.putULong("heap", ESP.getFreeHeap());
	prefs.putBool("valid", true);
	prefs.end();
}

CrashContextData loadCrashContext() {
	CrashContextData ctx;
	ctx.valid = false;
	ctx.section = 0;
	ctx.uptime_sec = 0;
	ctx.free_heap = 0;
	
	Preferences prefs;
	prefs.begin("crash", true);
	ctx.valid = prefs.getBool("valid", false);
	if (ctx.valid) {
		ctx.section = prefs.getUChar("section", 0);
		ctx.uptime_sec = prefs.getULong("uptime", 0);
		ctx.free_heap = prefs.getULong("heap", 0);
	}
	prefs.end();
	
	Preferences prefs2;
	prefs2.begin("crash", false);
	prefs2.putBool("valid", false);
	prefs2.end();
	
	return ctx;
}

#if defined(USE_SD_CARD) && defined(ALTRUIST_INSIDE)
// Write a boot diagnostic file to SD card with crash context from NVS
void writeBootFile() {
	if (!deviceStatus.sd_card_connected) return;
	
	// Build boot file path
	String bootPath = String(EXCEPTIONS_FOLDER) + "/boot_" + String(system_metrics.boot_counter) + ".txt";
	
	// Get reset reason
	esp_reset_reason_t reason = esp_reset_reason();
	
	// Load crash context from NVS (saved before crash)
	CrashContextData ctx = loadCrashContext();
	
	// Build content
	String content;
	content.reserve(512);
	
	content += "reset_reason: ";
	content += get_reset_reason_text();
	content += "\n";
	
	content += "reset_reason_code: ";
	content += String((int)reason);
	content += "\n";
	
	content += "boot_counter: ";
	content += String(system_metrics.boot_counter);
	content += "\n";
	
	content += "crash_data_valid: ";
	content += ctx.valid ? "yes" : "no (first boot or no saved context)";
	content += "\n";
	
	if (ctx.valid) {
		content += "prev_uptime_sec: ";
		content += String(ctx.uptime_sec);
		content += "\n";
		
		content += "prev_free_heap: ";
		content += String(ctx.free_heap);
		content += "\n";
		
		content += "last_section_id: ";
		content += String(ctx.section);
		content += "\n";
		
		content += "last_section_name: ";
		content += getCrashSectionName(ctx.section);
		content += "\n";
	} else {
		content += "prev_uptime_sec: 0\n";
		content += "prev_free_heap: 0\n";
		content += "last_section_id: 0\n";
		content += "last_section_name: N/A (no saved context)\n";
	}
	
	content += "current_free_heap: ";
	content += String(ESP.getFreeHeap());
	content += "\n";
	
	content += "rssi: ";
	content += String(WiFi.RSSI());
	content += "\n";
	
	// Write the file
	if (sdCardLogger.writeTextFile(bootPath, content)) {
		debug_outln_info(F("[Boot] Wrote crash diagnostic to: "), bootPath);
	} else {
		debug_outln_error(F("[Boot] Failed to write crash diagnostic"));
	}
}

#if defined(DEV)
// Background worker that continuously drains Debug logs to SD card so we
// can inspect them later even without a serial connection.
// Only active in dev builds to avoid unnecessary SD card wear in production.
static void exceptionsLogWorker(void *pvParameters) {
	(void)pvParameters;
	const String logPath  = String(EXCEPTIONS_FOLDER) + "/runtime.log";
	const String logPath1 = String(EXCEPTIONS_FOLDER) + "/runtime.log.1";
	const String logPath2 = String(EXCEPTIONS_FOLDER) + "/runtime.log.2";
	const size_t MAX_LOG_BYTES = 128UL * 1024UL; // 128KB
	String buffer;
	buffer.reserve(512);

	for (;;) {
		vTaskDelay(500 / portTICK_PERIOD_MS);

		if (!deviceStatus.sd_card_connected || !sdCardLogger.checkInserted()) {
			continue;
		}

		// Drain the in-memory debug queue into a small buffer, then append to SD.
		for (int i = 0; i < 20; i++) { // cap work per cycle
			String line = Debug.popLines();
			if (line.length() == 0) break;
			buffer += "[" + String(millis()) + "] " + line;
			if (buffer.length() > 800) break;
		}

		if (buffer.length() > 0) {
			// Rotate if the file would exceed the cap
			size_t currentSize = 0;
			if (sdCardLock(2000)) {
				if (SD.exists(logPath)) {
					File f = SD.open(logPath, FILE_READ);
					if (f) { currentSize = f.size(); f.close(); }
				}
				if (currentSize + buffer.length() > MAX_LOG_BYTES) {
					// Keep up to 2 backups: runtime.log -> runtime.log.1 -> runtime.log.2
					if (SD.exists(logPath2)) SD.remove(logPath2);
					if (SD.exists(logPath1)) SD.rename(logPath1, logPath2);
					if (SD.exists(logPath))  SD.rename(logPath,  logPath1);
				}
				sdCardUnlock();
			}
			if (currentSize + buffer.length() > MAX_LOG_BYTES) {
				// Start a new file with a simple header
				String header;
				header.reserve(128);
				header += "\n--- rotated at ms=";
				header += String(millis());
				header += " ---\n";
				sdCardLogger.writeTextFile(logPath, header);
			}

			sdCardLogger.appendTextFile(logPath, buffer);
			buffer = "";
		}
	}
}
#endif // DEV

// Periodic retention worker:
// - keep recent sensor CSV files for graph pages
// - keep only a bounded number of boot diagnostics in /exceptions
static void sdRetentionWorker(void *pvParameters) {
	(void)pvParameters;
	const uint16_t RAW_RETENTION_DAYS = 30;
	const uint16_t DAILY_ROLLUP_RETENTION_DAYS = 180;
	const uint16_t HOURLY_ROLLUP_RETENTION_DAYS = 30;
	const uint16_t MONTHLY_ROLLUP_RETENTION_MONTHS = 24;
	const uint16_t MAX_BOOT_FILES = 30;
	const unsigned long CLEANUP_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL; // 6 hours
	unsigned long last_cleanup_ms = 0;

	for (;;) {
		vTaskDelay(60000 / portTICK_PERIOD_MS); // check once per minute

		if (!deviceStatus.sd_card_connected || !sdCardLogger.checkInserted()) {
			continue;
		}

		if (last_cleanup_ms == 0 || msSince(last_cleanup_ms) > CLEANUP_INTERVAL_MS) {
			last_cleanup_ms = millis();
			debug_outln_info(F("[SDCardLogger] Running retention cleanup..."));
			sdCardLogger.buildDailyRollupsIfNeeded();
			sdCardLogger.buildMonthlyRollupsIfNeeded();
			sdCardLogger.applyRetentionPolicy(
				RAW_RETENTION_DAYS,
				DAILY_ROLLUP_RETENTION_DAYS,
				HOURLY_ROLLUP_RETENTION_DAYS,
				MONTHLY_ROLLUP_RETENTION_MONTHS,
				MAX_BOOT_FILES
			);
		}
	}
}
#endif // USE_SD_CARD && ALTRUIST_INSIDE

/*****************************************************************
 * Variables for Robonomics                                      *
 *****************************************************************/
Robonomics robonomics;

const int maxSensors = 10;
Sensor* activeSensors[maxSensors];
int activeSensorsCount = 0;

static void powerOnTestSensors() {

	debug_outln_info(F("Current reg: "), cfg::current_reg);

	for (int i = 0; i < sizeof(supported_sensor_names) / sizeof(supported_sensor_names[0]); i++) {
#if defined(ALTRUIST_INSIDE)
		if (cfg::standalone && supported_sensor_names[i] == HTTP_ALTRUIST_SENSOR_NAME) {
			debug_outln_info(F("Skipping Urban HTTP sensor (standalone mode)"));
			continue;
		}
#endif
		Sensor* new_sensor = createSensor(supported_sensor_names[i], cfg::sending_intervall_ms);
		if (new_sensor->begin()) {
			activeSensors[activeSensorsCount] = new_sensor;
			activeSensorsCount++;
			deviceStatus.sensor_names.push_back(new_sensor->sensor_name);
			debug_outln_info(F("Sensor was added: "), supported_sensor_names[i]);
		} else {
			debug_outln_info(F("Sensor was not added: "), supported_sensor_names[i]);
		}
	}

}

const int APIsCount = 3;
int ActiveAPIsCount = 2;
API* activeAPIs[APIsCount];

RobonomicsDatalogAPI robonomicsDatalogAPI;
RobonomicsHTTPAPI robonomicsHTTPAPI;
CustomHTTPAPI* customHTTPAPI = nullptr;

static void setupEnabledAPIs() {
	debug_outln_info(F("Send to :"));

	robonomicsDatalogAPI.setRobonomcis(&robonomics);
	robonomicsHTTPAPI.setRobonomcis(&robonomics);

	activeAPIs[0] = &robonomicsDatalogAPI;
	activeAPIs[1] = &robonomicsHTTPAPI;

	if (cfg::send2custom) {
		customHTTPAPI = new CustomHTTPAPI();
		customHTTPAPI->setRobonomcis(&robonomics);
		activeAPIs[2] = customHTTPAPI;
		ActiveAPIsCount++;
	}

	for (int i = 0; i < ActiveAPIsCount; i++) {
		activeAPIs[i]->setup();
		activeAPIs[i]->updateDeviceStatus(deviceStatus);
	}

}

#if defined(ESP32)
// Workaround for a core bug in Arduino-ESP32 configTzTime()/configTime()
// when CONFIG_LWIP_TCPIP_CORE_LOCKING is enabled: it may UNLOCK_TCPIP_CORE()
// even if it never LOCKed it, causing sys_mutex_unlock asserts on ESP32-C6.
static void safeConfigTzTime(const char* tz, const char* server1, const char* server2, const char* server3) {
	esp_netif_init();

	bool locked = false;
#ifdef CONFIG_LWIP_TCPIP_CORE_LOCKING
	if (!sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER)) {
		LOCK_TCPIP_CORE();
		locked = true;
	}
#endif

	if (sntp_enabled()) {
		sntp_stop();
	}

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, (char*)server1);
	sntp_setservername(1, (char*)server2);
	sntp_setservername(2, (char*)server3);
	sntp_init();

#ifdef CONFIG_LWIP_TCPIP_CORE_LOCKING
	if (locked) {
		UNLOCK_TCPIP_CORE();
	}
#endif

	setenv("TZ", tz, 1);
	tzset();
}
#endif

static void setupNetworkTime() {
	// server name ptrs must be persisted after the call to configTime because internally
	// the pointers are stored see implementation of lwip sntp_setservername()
	debug_outln_info(F("Setup time, timezone: "), cfg::timezone);

	static char ntpServer1[18], ntpServer2[18];
	strcpy_P(ntpServer1, NTP_SERVER_1);
	strcpy_P(ntpServer2, NTP_SERVER_2);

#if defined(ESP32)
	// Use safe SNTP init to avoid lwIP core-locking asserts on ESP32-C6.
	safeConfigTzTime(cfg::timezone, ntpServer1, ntpServer2, "");
#else
	configTzTime(cfg::timezone, ntpServer1, ntpServer2);
#endif
}

static void extractAnalyticsRollupValuesFromSensors(const DynamicJsonDocument &doc, analytics_screen_values_t &values) {
#if defined(ALTRUIST_INSIDE)
	values = analytics_screen_values_t{};
	JsonObjectConst data = doc.as<JsonObjectConst>();
	const String urban_key = ATRUIST_URBAN_SENSOR;

	auto validTemp = [](float v) { return v > -40.0f && v < 80.0f; };
	auto validHumidity = [](float v) { return v >= 0.0f && v <= 100.0f; };
	auto validCO2 = [](float v) { return v >= 300.0f && v <= 5000.0f; };
	auto validPM = [](float v) { return v >= 0.0f && v <= 1500.0f; };
	auto validNoise = [](float v) { return v >= 0.0f && v <= 120.0f; };

	const bool use_bme680_for_temp_hum = (system_metrics.uptime_sec < 360);

	if (data.containsKey("SCD4x")) {
		JsonObjectConst scd = data["SCD4x"].as<JsonObjectConst>();
		if (scd.containsKey("co2")) {
			const float v = scd["co2"]["value"].as<float>();
			if (validCO2(v)) { values.co2.current = v; values.co2.has_current = true; }
		}
		if (!use_bme680_for_temp_hum) {
			if (scd.containsKey("temperature")) {
				const float v = scd["temperature"]["value"].as<float>();
				if (validTemp(v)) { values.temp_indoor.current = v; values.temp_indoor.has_current = true; }
			}
			if (scd.containsKey("humidity")) {
				const float v = scd["humidity"]["value"].as<float>();
				if (validHumidity(v)) { values.hum_indoor.current = v; values.hum_indoor.has_current = true; }
			}
		}
	}

	if (data.containsKey("BME680") && use_bme680_for_temp_hum) {
		JsonObjectConst bme = data["BME680"].as<JsonObjectConst>();
		if (bme.containsKey("temperature")) {
			const float v = bme["temperature"]["value"].as<float>();
			if (validTemp(v)) { values.temp_indoor.current = v; values.temp_indoor.has_current = true; }
		}
		if (bme.containsKey("humidity")) {
			const float v = bme["humidity"]["value"].as<float>();
			if (validHumidity(v)) { values.hum_indoor.current = v; values.hum_indoor.has_current = true; }
		}
	}

	if (data.containsKey(urban_key)) {
		JsonObjectConst urban = data[urban_key].as<JsonObjectConst>();
		if (urban.containsKey("SDS_P2")) {
			const float v = urban["SDS_P2"]["value"].as<float>();
			if (validPM(v)) { values.pm25.current = v; values.pm25.has_current = true; }
		}
		if (urban.containsKey("PCBA_noiseAvg")) {
			const float v = urban["PCBA_noiseAvg"]["value"].as<float>();
			if (validNoise(v)) { values.noise_avg.current = v; values.noise_avg.has_current = true; }
		}
	}

	if (values.temp_indoor.has_current && values.hum_indoor.has_current && values.hum_indoor.current > 0.0f) {
		const float t = values.temp_indoor.current;
		const float h = values.hum_indoor.current;
		const float a = 17.62f;
		const float b = 243.12f;
		const float gamma = logf(h / 100.0f) + (a * t) / (b + t);
		const float dew = (b * gamma) / (a - gamma);
		if (validTemp(dew)) {
			values.dew_indoor.current = dew;
			values.dew_indoor.has_current = true;
		}
	}
#else
	(void)doc;
	(void)values;
#endif
}

bool fetchSensors() {
	bool any_sensor_json_updated = false;
	bool isSDSRunning = false;
		for (int i = 0; i < activeSensorsCount; i++) {
			if (activeSensors[i]->sensor_name == SDS_SENSOR_NAME) {
				isSDSRunning = static_cast<SDS011Sensor*>(activeSensors[i])->getIsSDSRunning();
			}
			if (activeSensors[i]->sensor_name == I2S_NOISE_SENSOR_NAME) {
				static_cast<I2SNoiseSensor*>(activeSensors[i])->setSDSRunning(isSDSRunning);
			}
			if (activeSensors[i]->isTimeToFetch()) {
				bool sensor_json_updated = false;
				if (xSemaphoreTake(mutex, portMAX_DELAY)) {
					activeSensors[i]->fetch(sensors_data);
					sensor_json_updated = activeSensors[i]->jsonUpdated();
					if (sensor_json_updated) {
						any_sensor_json_updated = true;
					}
					if (sensors_data.overflowed()) {
						debug_outln_error(F("[Sensors] JSON overflow after fetch"));
						debug_outln_info(F("[Sensors] sensor: "), String(activeSensors[i]->sensor_name));
						debug_outln_info(F("[Sensors] memory/capacity: "),
						                 String(sensors_data.memoryUsage()) + "/" + String(sensors_data.capacity()));
					}
					xSemaphoreGive(mutex);
				}
#if defined(USE_SD_CARD)
				// SD logging outside mutex - SD writes are slow and would block display
				deviceStatus.sd_card_connected = sdCardLogger.checkInserted();
				if (deviceStatus.sd_card_connected && sensor_json_updated) {
					sdCardLogger.logData(activeSensors[i]->sensor_name, sensors_data);
				}
#endif
			}
		}
	return any_sensor_json_updated;
}

void sensorAndAPIWorker(void *pvParameters) {
	int reconnected = 0;
#if defined(ALTRUIST_INSIDE)
	analytics_screen_values_t analytics_rollup_values;
#endif
	static bool prev_sta_connected = false;
		for (;;) {  // infinite loop
		if (wifiTakeUserPortalRequest()) {
#ifdef ALTRUIST_INSIDE
			displayManager.setScreen(ScreenPage::SETUP);
			displayManager.process(btn_press);
#endif
#ifdef ALTRUIST_URBAN
			leds_controller_urban.setMode(LedMode::PROVISIONING);
			leds_controller_urban.process();
#endif
			wifiConfig(webserver);
		}
#if defined(ESP32)
		// React to driver disconnect immediately (do not wait only for the 12s periodic timer).
		if (wifiStaTakeDisconnectReconnectKick() && wifiHasSavedStationCredentials() && !wifiIsConfigPortalRunning()) {
			debug_outln_info(F("[WiFi] STA disconnect event -> runtime STA recovery"));
			wifiStaRuntimeRecovery(false);
		}
		// Every DHCP IPv4 assignment: re-attach HTTP listener + mDNS. Fixes "http://192.168.x.x/ dead until reboot"
		// when the link looked continuously "ready" to our edge detector or lwIP replaced the STA netif under us.
		if (wifiStaTakeStaGotIpWebRefreshKick() && wifiHasSavedStationCredentials() && !wifiIsConfigPortalRunning() &&
		    WiFi.localIP()[0] != 0) {
			debug_outln_info(F("[WiFi] STA_GOT_IP -> refresh mDNS + web listener"));
			deviceStatus.ip_address = WiFi.localIP().toString();
			MDNS.end();
			if (MDNS.begin(cfg::local_hostname)) {
				MDNS.addService("altruist", "tcp", 80);
				MDNS.addServiceTxt("altruist", "tcp", "PATH", "/config");
				MDNS.addServiceTxt("altruist", "tcp", DEVICE_MODEL_MDNS_PROPERTY, DEVICE_MODEL);
			}
			webserver.notifyStaIpRestored();
		}
#endif
		// Run STA recovery before fetch/API work: slow sensors or HTTP can block for a long time and
		// would otherwise starve reconnect logic (LAN + webserver appeared "dead" until manual reboot).
		{
			static unsigned long last_periodic_wifi_recover_ms = 0;
			static unsigned long sta_down_since_ms = 0;
			if (!wifiStaLinkReady()) {
				if (sta_down_since_ms == 0) {
					sta_down_since_ms = millis();
				}
				const unsigned long now_ms = millis();
				if (last_periodic_wifi_recover_ms == 0 ||
				    msSince(last_periodic_wifi_recover_ms) >= WIFI_STA_PERIODIC_RECONNECT_MS) {
					markCrashSection(CRASH_SECTION_WIFI_RECONNECT);
					const bool want_deep = (msSince(sta_down_since_ms) >= WIFI_STA_DEEP_RECOVER_AFTER_MS);
					if (wifiStaRuntimeRecovery(want_deep)) {
						last_periodic_wifi_recover_ms = now_ms;
					}
					if (WIFI_STA_REBOOT_AFTER_MS != 0 && wifiHasSavedStationCredentials() &&
					    msSince(sta_down_since_ms) >= WIFI_STA_REBOOT_AFTER_MS) {
						debug_outln_info(F("[WiFi] STA link down too long; rebooting for recovery"));
						delay(100);
						esp_restart();
					}
					markCrashSection(CRASH_SECTION_IDLE);
				}
			} else {
				last_periodic_wifi_recover_ms = 0;
				sta_down_since_ms = 0;
			}
		}
		// If STA comes back after an outage, re-advertise mDNS so .local URLs start working again.
		// Without this, the webserver may be reachable by IP but appears "down" when using hostname.
		{
			const bool sta_connected_now = wifiStaLinkReady();
			if (sta_connected_now && !prev_sta_connected) {
				deviceStatus.ip_address = WiFi.localIP().toString();
				MDNS.end();
				if (MDNS.begin(cfg::local_hostname)) {
					MDNS.addService("altruist", "tcp", 80);
					MDNS.addServiceTxt("altruist", "tcp", "PATH", "/config");
					MDNS.addServiceTxt("altruist", "tcp", DEVICE_MODEL_MDNS_PROPERTY, DEVICE_MODEL);
				}
				webserver.notifyStaIpRestored();
			} else if (!sta_connected_now && prev_sta_connected) {
				MDNS.end();
			}
			prev_sta_connected = sta_connected_now;
		}
		// Mark that we're about to fetch sensor data
		markCrashSection(CRASH_SECTION_FETCH_SENSORS);
		const bool sensors_updated = fetchSensors();
#if defined(ALTRUIST_INSIDE)
		// Keep analytics history collection independent from Analytics screen rendering.
		if (sensors_updated && xSemaphoreTake(mutex, pdMS_TO_TICKS(200))) {
			extractAnalyticsRollupValuesFromSensors(sensors_data, analytics_rollup_values);
			analyticsIngestHourSample(analytics_rollup_values);
			xSemaphoreGive(mutex);
		}
		analyticsDevLogStatus15m();
#endif
		markCrashSection(CRASH_SECTION_IDLE);

		for (int i = 0; i < ActiveAPIsCount; i++) {
			if (activeAPIs[i]->isTimeToSend()) {
			#ifdef DEV
			#if defined(ALTRUIST_INSIDE)
			Serial.printf("[INSIGHT] WiFi status connected: %d, reconnected: %d\r\n", WiFi.status() == WL_CONNECTED, reconnected);
			#elif defined(ALTRUIST_URBAN)
			Serial.printf("[URBAN] WiFi status connected: %d, reconnected: %d\r\n", WiFi.status() == WL_CONNECTED, reconnected);
			#endif
			#endif

			if (!wifiStaLinkReady()) {
    		continue;
			}

			// Mark based on which API we're sending to
			// API index 0 is typically Robonomics Datalog, 1 is HTTP Map, etc.
			if (i == 0) {
				markCrashSection(CRASH_SECTION_ROBONOMICS_DATALOG);
			} else if (i == 1) {
				markCrashSection(CRASH_SECTION_ROBONOMICS_HTTP_MAP);
			} else {
				markCrashSection(CRASH_SECTION_CUSTOM_HTTP);
			}
			
			DynamicJsonDocument api_snapshot(SENSORS_JSON_CAPACITY);
			bool snapshot_ok = false;
			if (xSemaphoreTake(mutex, portMAX_DELAY)) {
				sensors_data["service_data"]["signal_strength"] = WiFi.RSSI();
				api_snapshot.set(sensors_data.as<JsonVariantConst>());
				snapshot_ok = !api_snapshot.overflowed();
				if (!snapshot_ok) {
					debug_outln_error(F("[API] JSON snapshot overflow; skipping send"));
					debug_outln_info(F("[API] memory/capacity: "),
					                 String(api_snapshot.memoryUsage()) + "/" + String(api_snapshot.capacity()));
				}
				xSemaphoreGive(mutex);
			}
			if (!snapshot_ok) {
				continue;
			}
#ifdef ALTRUIST_URBAN
			const bool urban_is_datalog_send = (activeAPIs[i] == &robonomicsDatalogAPI);
			if (urban_is_datalog_send) {
				urban_datalog_result = 0;
				urban_datalog_sending = true;
				leds_controller_urban.setMode(LedMode::BLUE);
				leds_controller_urban.process();
			}
#endif
			activeAPIs[i]->send(api_snapshot);
#ifdef ALTRUIST_URBAN
			if (urban_is_datalog_send) {
				urban_datalog_sending = false;
				urban_datalog_result = activeAPIs[i]->lastSendWasOk() ? 1 : -1;
				urban_datalog_result_until_ms = millis() + 3000UL;
				leds_controller_urban.setMode(urban_datalog_result > 0 ? LedMode::GREEN : LedMode::RED);
				leds_controller_urban.process();
			}
#endif
			incrementTXCounter(); // Track successful telemetry send
			activeAPIs[i]->updateDeviceStatus(deviceStatus);
			
			markCrashSection(CRASH_SECTION_IDLE);
				
			static bool first_ota_check = true;
			bool manual_ota = deviceStatus.ota_update_requested;
			if (manual_ota) {
				deviceStatus.ota_update_requested = false;
			}
			if (wifiStaLinkReady() &&
					(first_ota_check || manual_ota || msSince(deviceStatus.last_update_attempt) > PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS)) {

					first_ota_check = false;
					twoStageOTAUpdate(deviceStatus, manual_ota);
					deviceStatus.last_update_attempt = millis();
			}


			#ifdef DEV
			#if defined(ALTRUIST_INSIDE)
			Serial.println(F("[INSIGHT] Device Status:"));
			#elif defined(ALTRUIST_URBAN)
			Serial.println(F("[URBAN] Device Status:"));
			#endif
			#endif
			bool senders_ok = true;
			for (const auto& [api_name, status] : deviceStatus.apis_status) {
				#ifdef DEV
				Serial.print(F("API Name: "));
				Serial.println(api_name.c_str());
				Serial.print(F("  Count Sends: "));
				Serial.println(status.count_sends);
				Serial.print(F("  Last Send Time: "));
				Serial.println(ctime(&status.last_send_time));
				Serial.print(F("  Is OK: "));
				Serial.println(status.is_ok ? F("Yes") : F("No"));
				#endif
				senders_ok = senders_ok && status.is_ok;
			}
#ifdef ALTRUIST_URBAN
			urban_api_status_ok = senders_ok;
#endif
			}
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);  // yield to other tasks, run ~10x/sec
	}
}

void ledsWorker(void *pvParameters) {
	for (;;) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
		markCrashSection(CRASH_SECTION_LED_UPDATE);
#ifdef ALTRUIST_URBAN
		const bool urban_has_saved_wifi = wifiHasSavedStationCredentials();
		const bool urban_config_mode = !urban_has_saved_wifi || wifiIsConfigPortalRunning();
		const bool urban_error_now = urban_has_saved_wifi && (!wifiStaLinkReady() || !urban_api_status_ok);
		const unsigned long urban_now_ms = millis();
		const bool urban_datalog_result_active = urban_datalog_result != 0 &&
			(long)(urban_datalog_result_until_ms - urban_now_ms) > 0;
		static unsigned long urban_error_since_ms = 0;

		LedMode urban_led_mode = LedMode::GREEN;
		if (urban_datalog_sending) {
			urban_led_mode = LedMode::BLUE;
		} else if (urban_datalog_result_active) {
			urban_led_mode = urban_datalog_result > 0 ? LedMode::GREEN : LedMode::RED;
		} else if (urban_config_mode) {
			urban_error_since_ms = 0;
			urban_datalog_result = 0;
			urban_led_mode = LedMode::PROVISIONING;
		} else if (urban_error_now) {
			if (urban_error_since_ms == 0) {
				urban_error_since_ms = millis();
			}
			urban_datalog_result = 0;
			urban_led_mode = msSince(urban_error_since_ms) > 10UL * 60UL * 1000UL ? LedMode::RED : LedMode::GREEN;
		} else {
			urban_error_since_ms = 0;
			urban_datalog_result = 0;
			urban_led_mode = LedMode::GREEN;
		}

		leds_controller_urban.setMode(urban_led_mode);
		leds_controller_urban.process();
#endif
#ifdef ALTRUIST_INSIDE
		leds_controller_insight.process();
#endif
		markCrashSection(CRASH_SECTION_IDLE);
	}
}

void buttonsWorker(void *pvParameters) {
#ifdef ALTRUIST_URBAN
	static bool reset_pressed = false;
	static bool reset_armed = false;
	static unsigned long reset_press_start_ms = 0;
	static int reset_idle_level = -1;
#endif
	for (;;) {
		button_pressed_t res = button_manager.process();
		vTaskDelay(10 / portTICK_PERIOD_MS);
#ifdef ALTRUIST_URBAN
		// Dedicated Urban reset button on GPIO7.
		// We treat "pressed" as a change from idle level so it works for both:
		// - active-low wiring (button -> GND, INPUT_PULLUP)
		// - active-high wiring (button -> 3V3, external pulldown)
		if (URBAN_RESET_BTN_PIN != -1) {
			int level = digitalRead(URBAN_RESET_BTN_PIN);
			if (reset_idle_level == -1) {
				reset_idle_level = level;
				debug_outln_info(F("[RESET] GPIO idle level: "), String(reset_idle_level));
			}
			bool pressed_now = (level != reset_idle_level);
			if (pressed_now && !reset_pressed) {
				reset_pressed = true;
				reset_armed = false;
				reset_press_start_ms = millis();
			} else if (!pressed_now && reset_pressed) {
				// Release edge: if long-press was armed, execute reset on release.
				if (reset_armed) {
					debug_outln_info(F("[RESET] Confirmed by release, wiping WiFi + Web UI creds and opening Wi-Fi portal"));
					leds_controller_urban.setMode(LedMode::RESETTING);
					leds_controller_urban.process();
					removeWiFiCredentials();
					removeWebUiCredentials();
					delay(200);
					WiFi.disconnect(true);
					delay(200);
					requestWifiConfigPortal();
				}
				reset_pressed = false;
				reset_armed = false;
			}
			if (reset_pressed && !reset_armed) {
				if (msSince(reset_press_start_ms) > 5000) {
					// Arm reset, require release to confirm (prevents stuck-low pin wipe).
					reset_armed = true;
					debug_outln_info(F("[RESET] Armed (hold >5s). Release to confirm."));
				}
			}
		}
#endif
		if (res.pressed) {
			btn_press.button_num = res.button_num;
			btn_press.press_type = res.press_type;
			btn_press.double_long = res.double_long;
			btn_press.second_button_num = res.second_button_num;
			btn_press.pressed = true;
#ifdef ALTRUIST_INSIDE
			if (btn_press.double_long) {
				debug_outln_info(F("[WiFi] Insight SET+DOWN long: wipe Wi-Fi, show cleared + restart, then setup AP"));
				removeWiFiCredentials();
				btn_press.pressed = false;
				btn_press.double_long = false;
				WiFi.disconnect(true);
				displayManager.setScreen(ScreenPage::LOGO);
				displayManager.process(btn_press);
				delay(2500);
				set_restart_reason(RESTART_REASON_CONFIG);
				esp_restart();
			}
#endif
		}
	}
}

#ifdef DEV
void metricsWorker(void *pvParameters) {
	// Wait a bit for system to stabilize
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	
	// Log first metrics immediately and save initial crash context
	logMetrics();
	saveCrashContext();
	
	// Track when we last saved crash context
	static unsigned long last_crash_save_ms = 0;
	const unsigned long CRASH_SAVE_INTERVAL_MS = 30UL * 1000UL; // save every 30s
	
	// Then log every 3 seconds
	for (;;) {
		vTaskDelay(3000 / portTICK_PERIOD_MS);  // Wait 3 seconds
		
		// Periodic memory telemetry: log remaining heap so we can see trends
		// and potential leaks over hours/days in the SD runtime log.
#if defined(ESP32)
		static unsigned long last_mem_log_ms = 0;
		const unsigned long MEM_LOG_INTERVAL_MS = 60UL * 1000UL; // log every 60s
		uint32_t free_heap = ESP.getFreeHeap();
		if (msSince(last_mem_log_ms) > MEM_LOG_INTERVAL_MS) {
			last_mem_log_ms = millis();
			debug_outln_info(F("[MEM] Free heap bytes"), String(free_heap));
			if (free_heap < 50000) {
				debug_outln_info(F("[MEM][WARN] Low free heap bytes"), String(free_heap));
			}
#if defined(USE_SD_CARD)
			uint32_t sd_ok = 0, sd_fail = 0, sd_busy = 0;
			static uint32_t prev_sd_ok = 0, prev_sd_fail = 0, prev_sd_busy = 0;
			sdGetDevCounters(sd_ok, sd_fail, sd_busy);
			debug_outln_info(F("[SD][DEV] csv writes (+)"), String(sd_ok - prev_sd_ok));
			debug_outln_info(F("[SD][DEV] csv write fails (+)"), String(sd_fail - prev_sd_fail));
			debug_outln_info(F("[SD][DEV] sd lock busy (+)"), String(sd_busy - prev_sd_busy));
			prev_sd_ok = sd_ok;
			prev_sd_fail = sd_fail;
			prev_sd_busy = sd_busy;
#endif
		}
#endif

		logMetrics();
		
		if (msSince(last_crash_save_ms) > CRASH_SAVE_INTERVAL_MS) {
			last_crash_save_ms = millis();
			saveCrashContext();
		}
	}
}
#endif


void setup(void) {
#ifdef ALTRUIST_INSIDE
	bool insight_sta_join_started_early = false;
#endif
	delay(300);
	Serial.begin(115200);
	delay(500);
	#ifdef DEV
	#if defined(ALTRUIST_INSIDE)
	Serial.println(F("[INSIGHT] Start setup"));
	#elif defined(ALTRUIST_URBAN)
	Serial.println(F("[URBAN] Start setup"));
	#endif
	Serial.flush();
	#endif
	delay(200);

	// If SET button pressed while turn on, reset the configuration
#ifdef ALTRUIST_URBAN
	leds_controller_urban.init();
	if (URBAN_RESET_BTN_PIN != -1) {
		pinMode(URBAN_RESET_BTN_PIN, INPUT_PULLUP);
	}
#endif
#ifdef ALTRUIST_INSIDE
	leds_controller_insight.init();
#endif
	button_manager.init();
	bool reset_needed = true;
	for (int i=0; i<5; i++) {
		button_manager.process();
#ifdef ALTRUIST_URBAN
		reset_needed = reset_needed && (button_manager.get_button_state(ButtonNum::SET) == PRESSED_STATE);
#endif
#ifdef ALTRUIST_INSIDE
		reset_needed = reset_needed && button_manager.get_button_state(ButtonNum::SET) == PRESSED_STATE && button_manager.get_button_state(ButtonNum::DOWN) == PRESSED_STATE;
#endif
		delay(10);
	}
	if (reset_needed) {
		debug_outln_info(F("Delete configuration and restart"));
		init_config();
		SPIFFS.remove(F("/config.json.old"));
		SPIFFS.remove(F("/config.json"));
		delay(2000);
		// esp_restart();
	}

#ifdef ALTRUIST_INSIDE
	DEV_Module_Init();
	displayManager.setup();
	displayManager.setScreen(ScreenPage::LOADING);
	displayManager.process(btn_press);
#endif

	String esp_chipid = get_chipid();
	cfg::initNonTrivials(esp_chipid.c_str());
	WiFi.persistent(false);
#if defined(ESP32)
	wifiRegisterStaRecoveryEvents();
#endif
	
	// Initialize metrics (load boot counter, etc.) - works for both Urban and Insight
	#ifdef DEV
	#if defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT][Setup] Initializing metrics system...\r\n"));
	#elif defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN][Setup] Initializing metrics system...\r\n"));
	#endif
	#endif
	Serial.flush();
	delay(10);
	initMetrics();
	#ifdef DEV
	#if defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT][Setup] Metrics initialized. Boot counter: "));
	#elif defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN][Setup] Metrics initialized. Boot counter: "));
	#endif
	Serial.print(system_metrics.boot_counter);
	Serial.println();
	#endif
	Serial.flush();
	delay(10);
	
	// Initialize ESP32-C6 temperature sensor
	#ifdef DEV
	#if defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN][Setup] Initializing ESP temperature sensor...\r\n"));
	#elif defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT][Setup] Initializing ESP temperature sensor...\r\n"));
	#endif
	Serial.flush();
	#endif
	delay(10);
	initESPTemperatureSensor();
	#ifdef DEV
	#if defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN][Setup] ESP temperature sensor initialized\r\n"));
	#elif defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT][Setup] ESP temperature sensor initialized\r\n"));
	#endif
	#endif
	Serial.flush();
	delay(10);

	debug_outln_info(F("Altruist: " SOFTWARE_VERSION_STR "/"), String(CURRENT_LANG));

	init_config();
#ifdef ALTRUIST_URBAN
	leds_controller_urban.setMode(wifiHasSavedStationCredentials() ? LedMode::GREEN : LedMode::PROVISIONING);
	leds_controller_urban.process();
#endif
	// Sync config language with actual compiled firmware language.
	// Covers: USB reflash with different locale, failed OTA language switch.
#ifdef ALTRUIST_INSIDE
	if (wifiHasSavedStationCredentials()) {
		debug_outln_info(F("[WiFi] Insight: early STA join (parallel with display / rest of setup)"));
		wifiStaBeginStationJoin();
		insight_sta_join_started_early = true;
	}
#endif
	if (strcmp(cfg::current_lang, CURRENT_LANG) != 0) {
		strcpy(cfg::current_lang, CURRENT_LANG);
		writeConfig();
	}
	setupEnabledAPIs();
	webserver.setRobonomicsAddress(robonomics.getSs58Address());
#ifdef ALTRUIST_INSIDE
	displayManager.setRobonomicsAddress(robonomics.getSs58Address());
	if (strcmp(cfg::wlanssid, WLANSSID) != 0) {
		displayManager.setScreen(ScreenPage::CONNECTING);
		displayManager.process(btn_press);
	}
#endif
	// Create button worker task BEFORE wifiConfig so buttons work during WiFi setup
	xTaskCreatePinnedToCore(
		buttonsWorker,  // task function
		"ButtonWorker",   // name
		2048,                // stack size
		NULL,                // parameters
		1,                   // priority (>=1 to not be preempted too much)
		NULL,                // task handle (optional)
		0                    // core 0 (ESP32-C3/C6 is single-core anyway)
	);
	if (!wifiHasSavedStationCredentials()) {
#ifdef ALTRUIST_URBAN
		leds_controller_urban.setMode(LedMode::PROVISIONING);
		leds_controller_urban.process();
#endif
		// Insight: SETUP e-ink is drawn inside wifiConfig() after AP + captive portal are up (same idea as Urban: RF first).
		wifiConfig(webserver);
	} else {
#ifdef ALTRUIST_INSIDE
		const bool sta_ok = connectWifi(webserver, insight_sta_join_started_early);
#else
		const bool sta_ok = connectWifi(webserver);
#endif
		if (!sta_ok) {
#ifdef ALTRUIST_INSIDE
			// Insight: wrong password / unreachable SSID must not leave the device "running" without
			// a working STA — same as first-time setup, block in the captive portal until WiFi works
			// (portal flow ends with sensor_restart() once association succeeds).
			debug_outln_info(F("[WiFi] Insight: STA did not connect with saved credentials; starting config portal."));
			wifiConfig(webserver);
#else
			debug_outln_info(F("[WiFi] Saved credentials but STA did not connect; skipping config AP (runtime reconnect)."));
#endif
		}
	}

	// Configure SNTP time only after TCP/IP + WiFi are up.
	// On ESP32-C6 we observed lwIP asserts when configTzTime() runs before WiFi is connected.
	if (wifiStaLinkReady()) {
		setupNetworkTime();
	}
	powerOnTestSensors();
	webserver.setup();
	debug_outln_info(F("\nChipId: "), esp_chipid);
	debug_outln_info(get_reset_reason_text());
	// OTA runs from sensorAndAPIWorker (not here) so display can show "Updating" screen

	sensors_data["service_data"]["robonomics_address"] = robonomics.getSs58Address();
	sensors_data["service_data"]["signal_strength"] = WiFi.RSSI();
#ifdef ALTRUIST_INSIDE
	// Pre-create Urban block at boot while the JSON document is still mostly empty.
	// This avoids allocation failures later when Urban appears after Insight has
	// already filled sensors_data with other keys.
	if (sensors_data[ATRUIST_URBAN_SENSOR].isNull()) {
		sensors_data.createNestedObject(ATRUIST_URBAN_SENSOR);
	}
#endif

	delay(50);

	debug_outln_info(F("Active Sensors count: "), activeSensorsCount);

	#ifdef DEV
	#if defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT] Sensors: "));
	#elif defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN] Sensors: "));
	#endif
    for (const auto &sensor : deviceStatus.sensor_names) {
        Serial.print(sensor.c_str());
        Serial.print(F(" "));
    }
    Serial.println();
	#endif

	deviceStatus.last_update_attempt = deviceStatus.time_point_device_start_ms = millis();
#if defined(USE_SD_CARD)
	deviceStatus.sd_card_connected = sdCardLogger.begin();
	if (deviceStatus.sd_card_connected) {
		debug_outln_info(F("[SDCardLogger] SD card connected"));
	} else {
		debug_outln_error(F("[SDCardLogger] SD card NOT connected"));
	}
#endif

#if defined(USE_SD_CARD) && defined(ALTRUIST_INSIDE)
	// Write boot diagnostic file immediately after SD init - captures RTC crash context
	writeBootFile();

	// SD retention cleanup worker (production + dev):
	// keeps exceptions and sensor CSV history bounded.
	xTaskCreatePinnedToCore(
		sdRetentionWorker,
		"SDRetentionWorker",
		4096,
		NULL,
		1,
		NULL,
		0
	);
	
#if defined(DEV)
	// Background logger: keep a rolling runtime log on SD so we have context
	// leading up to random panics/resets (panic backtrace itself is not SD-safe).
	// Only active in dev builds to avoid unnecessary SD card wear in production.
	xTaskCreatePinnedToCore(
		exceptionsLogWorker,
		"ExceptionsLogWorker",
		4096,
		NULL,
		1,
		NULL,
		0
	);
#endif // DEV
#endif // USE_SD_CARD && ALTRUIST_INSIDE
	fetchSensors();
	deviceStatus.ip_address = WiFi.localIP().toString();

#ifdef ALTRUIST_INSIDE
	// First MAIN + LEDs after a real sensor read (avoids empty metrics).
	// Draw LEDs before the long e-paper MAIN refresh: process() holds the mutex for seconds, so LED
	// would otherwise miss the 100 ms timeout and stay dark until scheduleNextLedRefresh.
	if (wifiHasSavedStationCredentials()) {
		displayManager.setScreen(ScreenPage::MAIN);
		leds_controller_insight.process();
		displayManager.process(btn_press);
		leds_controller_insight.scheduleNextLedRefresh(4000);
	}
#endif

#ifdef ALTRUIST_URBAN
	leds_controller_urban.setMode(LedMode::GREEN);
	leds_controller_urban.process();
#endif

	xTaskCreatePinnedToCore(
		sensorAndAPIWorker,  // task function
		"SensorAPIWorker",   // name
		16392,                // stack size
		NULL,                // parameters
		2,                   // priority (>=1 to not be preempted too much)
		NULL,                // task handle (optional)
		0                    // core 0 (ESP32-C3/C6 is single-core anyway)
	);
	// Note: buttonsWorker task is created earlier (before wifiConfig) so buttons work during WiFi setup
	xTaskCreatePinnedToCore(
		ledsWorker,  // task function
		"LedsWorker",   // name
		2048,                // stack size
		NULL,                // parameters
		2,                   // priority (>=1 to not be preempted too much)
		NULL,                // task handle (optional)
		0                    // core 0 (ESP32-C3/C6 is single-core anyway)
	);
	
	// Create metrics worker task only for DEV builds
	#ifdef DEV
	#if defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT][Setup] Creating metrics worker task...\r\n"));
	#elif defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN][Setup] Creating metrics worker task...\r\n"));
	#endif
	Serial.flush();
	delay(10);
	TaskHandle_t metricsTaskHandle = NULL;
	BaseType_t metricsTaskResult = xTaskCreatePinnedToCore(
		metricsWorker,  // task function
		"MetricsWorker",   // name
		4096,                // stack size
		NULL,                // parameters
		2,                   // priority
		&metricsTaskHandle,  // task handle
		0                    // core 0
	);
	if (metricsTaskResult != pdPASS) {
		#if defined(ALTRUIST_INSIDE)
		Serial.print(F("[INSIGHT][ERROR] Failed to create metrics worker task! Result: "));
		#elif defined(ALTRUIST_URBAN)
		Serial.print(F("[URBAN][ERROR] Failed to create metrics worker task! Result: "));
		#endif
		Serial.println(metricsTaskResult);
		Serial.flush();
	} else {
		#if defined(ALTRUIST_INSIDE)
		Serial.print(F("[INSIGHT][Setup] Metrics worker task created successfully! Handle: 0x"));
		#elif defined(ALTRUIST_URBAN)
		Serial.print(F("[URBAN][Setup] Metrics worker task created successfully! Handle: 0x"));
		#endif
		Serial.println((uint32_t)metricsTaskHandle, HEX);
		Serial.flush();
	}
	delay(10);
	#endif
	
	debug_outln_info(F("Setup finished"));
	#ifdef DEV
	#if defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT][Setup] All tasks created, testing logMetrics()...\r\n"));
	#elif defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN][Setup] All tasks created, testing logMetrics()...\r\n"));
	#endif
	Serial.flush();
	delay(10);
	logMetrics();
	#if defined(ALTRUIST_INSIDE)
	Serial.print(F("[INSIGHT][Setup] Metrics test complete\r\n"));
	#elif defined(ALTRUIST_URBAN)
	Serial.print(F("[URBAN][Setup] Metrics test complete\r\n"));
	#endif
	Serial.flush();
	#endif
	
	// button_controller.init();
}

void loop(void) {
	webserver.handleClient();
#if defined(ALTRUIST_INSIDE)
	markCrashSection(CRASH_SECTION_DISPLAY_UPDATE);
	displayManager.process(btn_press);
	markCrashSection(CRASH_SECTION_IDLE);
#endif
	yield();
}
