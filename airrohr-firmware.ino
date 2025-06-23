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

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#include <ArduinoJson.h>

#include "./intl.h"

#include "./utils.h"
#include "defines.h"
#include "ext_def.h"
#include "webserver/html-content.h"
#include <Robonomics.h>
#include "sensors/sensor_factory.h"
#include "apis/apis.h"
#include "config_manager/config_helpers.h"
#include "wifi_manager.h"
#include "webserver/webserver.h"
#include "OTA_Update.h"

String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);

SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
DynamicJsonDocument sensors_data(2048);
device_status_t deviceStatus;

SensorWebServer webserver(sensors_data, deviceStatus, mutex);

/*****************************************************************
 * Variables for Robonomics                                      *
 *****************************************************************/
Robonomics robonomics;


/*****************************************************************
 * send data to influxdb                                         *
 *****************************************************************/
// static void create_influxdb_string_from_data(String& data_4_influxdb, const String& data) {
// 	debug_outln_verbose(F("Parse JSON for influx DB: "), data);
// 	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
// 	DeserializationError err = deserializeJson(json2data, data);
// 	if (!err) {
// 		data_4_influxdb += cfg::measurement_name_influx;
// 		data_4_influxdb += F(",node=" SENSOR_BASENAME);
// 		data_4_influxdb += esp_chipid + " ";
// 		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>()) {
// 			data_4_influxdb += measurement["value_type"].as<char*>();
// 			data_4_influxdb += "=";

// 			if (isNumeric(measurement["value"])) {
// 				//send numerics without quotes
// 				data_4_influxdb += measurement["value"].as<char*>();
// 			} else {
// 				//quote string values
// 				data_4_influxdb += "\"";
// 				data_4_influxdb += measurement["value"].as<char*>();
// 				data_4_influxdb += "\"";
// 			}
// 			data_4_influxdb += ",";
// 		}
// 		if ((unsigned)(data_4_influxdb.lastIndexOf(',') + 1) == data_4_influxdb.length()) {
// 			data_4_influxdb.remove(data_4_influxdb.length() - 1);
// 		}

// 		data_4_influxdb += '\n';
// 	} else {
// 		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
// 	}
// }

/*****************************************************************
 * send data as csv to serial out                                *
 *****************************************************************/
// static void send_csv(const String& data) {
// 	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
// 	DeserializationError err = deserializeJson(json2data, data);
// 	debug_outln_info(F("CSV Output: "), data);
// 	if (!err) {
// 		String headline = F("Timestamp_ms;");
// 		String valueline(act_milli);
// 		valueline += ';';
// 		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>()) {
// 			headline += measurement["value_type"].as<char*>();
// 			headline += ';';
// 			valueline += measurement["value"].as<char*>();
// 			valueline += ';';
// 		}
// 		static bool first_csv_line = true;
// 		if (first_csv_line) {
// 			if (headline.length() > 0) {
// 				headline.remove(headline.length() - 1);
// 			}
// 			Debug.println(headline);
// 			first_csv_line = false;
// 		}
// 		if (valueline.length() > 0) {
// 			valueline.remove(valueline.length() - 1);
// 		}
// 		Debug.println(valueline);
// 	} else {
// 		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
// 	}
// }

/*****************************************************************
 * read DHT22 sensor values                                      *
 *****************************************************************/
// static void fetchSensorDHT(String& s) {
// 	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DHT22));

// 	// Check if valid number if non NaN (not a number) will be send.
// 	last_value_DHT_T = -128;
// 	last_value_DHT_H = -1;

// 	int count = 0;
// 	const int MAX_ATTEMPTS = 5;
// 	while ((count++ < MAX_ATTEMPTS)) {
// 		auto t = dht.readTemperature();
// 		auto h = dht.readHumidity();
// 		if (isnan(t) || isnan(h)) {
// 			delay(100);
// 			t = dht.readTemperature(false);
// 			h = dht.readHumidity();
// 		}
// 		if (isnan(t) || isnan(h)) {
// 			debug_outln_error(F("DHT11/DHT22 read failed"));
// 		} else {
// 			last_value_DHT_T = t + readCorrectionOffset(cfg::temp_correction);
// 			last_value_DHT_H = h;
// 			add_Value2Json(s, F("temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_DHT_T);
// 			add_Value2Json(s, F("humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_DHT_H);
// 			break;
// 		}
// 	}
// 	debug_outln_info(FPSTR(DBG_TXT_SEP));

// 	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DHT22));
// }

/*****************************************************************
 * read HTU21D sensor values                                     *
 *****************************************************************/
// static void fetchSensorHTU21D(String& s) {
// 	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_HTU21D));

// 	const auto t = htu21d.readTemperature();
// 	const auto h = htu21d.readHumidity();
// 	if (isnan(t) || isnan(h)) {
// 		last_value_HTU21D_T = -128.0;
// 		last_value_HTU21D_H = -1.0;
// 		debug_outln_error(F("HTU21D read failed"));
// 	} else {
// 		last_value_HTU21D_T = t;
// 		last_value_HTU21D_H = h;
// 		add_Value2Json(s, F("HTU21D_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_HTU21D_T);
// 		add_Value2Json(s, F("HTU21D_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_HTU21D_H);
// 	}
// 	debug_outln_info(FPSTR(DBG_TXT_SEP));

// 	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_HTU21D));
// }



const int maxSensors = 10;
Sensor* activeSensors[maxSensors];
int activeSensorsCount = 0;

static void powerOnTestSensors() {

	debug_outln_info(F("Current reg: "), cfg::current_reg);

	for (int i = 0; i < sizeof(supported_sensor_names) / sizeof(supported_sensor_names[0]); i++) {
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

static void setupNetworkTime() {
	// server name ptrs must be persisted after the call to configTime because internally
	// the pointers are stored see implementation of lwip sntp_setservername()
	static char ntpServer1[18], ntpServer2[18];
	strcpy_P(ntpServer1, NTP_SERVER_1);
	strcpy_P(ntpServer2, NTP_SERVER_2);
	configTime(0, 0, ntpServer1, ntpServer2);
}

void sensorAndAPIWorker(void *pvParameters) {
	int reconnected = 0;
	for (;;) {  // infinite loop
		bool isSDSRunning = false;
		for (int i = 0; i < activeSensorsCount; i++) {
			if (activeSensors[i]->sensor_name == SDS_SENSOR_NAME) {
				isSDSRunning = static_cast<SDS011Sensor*>(activeSensors[i])->getIsSDSRunning();
			}
			if (activeSensors[i]->sensor_name == I2S_NOISE_SENSOR_NAME) {
				static_cast<I2SNoiseSensor*>(activeSensors[i])->setSDSRunning(isSDSRunning);
			}
			if (activeSensors[i]->isTimeToFetch()) {
				if (xSemaphoreTake(mutex, portMAX_DELAY)) {
					activeSensors[i]->fetch(sensors_data);
					xSemaphoreGive(mutex);
				}
			}
		}

		for (int i = 0; i < ActiveAPIsCount; i++) {
			if (activeAPIs[i]->isTimeToSend()) {
			Serial.printf("WiFi status connected: %d, reconnected: %d\r\n", WiFi.status() == WL_CONNECTED, reconnected);
			if (WiFi.status() != WL_CONNECTED) {
				WiFi.reconnect();
				reconnected++;
			}

			sensors_data["service_data"]["signal_strength"] = WiFi.RSSI();
			activeAPIs[i]->send(sensors_data);
			activeAPIs[i]->updateDeviceStatus(deviceStatus);

			if (msSince(deviceStatus.last_update_attempt) > PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS) {
				twoStageOTAUpdate(deviceStatus);
				deviceStatus.last_update_attempt = millis();
			}

			debug_outln_info(get_reset_reason_text());

			Serial.println(F("Device Status:"));
			for (const auto& [api_name, status] : deviceStatus.apis_status) {
				Serial.print(F("API Name: "));
				Serial.println(api_name.c_str());
				Serial.print(F("  Count Sends: "));
				Serial.println(status.count_sends);
				Serial.print(F("  Last Send Time: "));
				Serial.println(ctime(&status.last_send_time));
				Serial.print(F("  Is OK: "));
				Serial.println(status.is_ok ? F("Yes") : F("No"));
			}

			sensors_data.shrinkToFit();
			}
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);  // yield to other tasks, run ~10x/sec
	}
}


/*****************************************************************
 * The Setup                                                     *
 *****************************************************************/

void setup(void) {
	delay(3000);
	// Debug.begin(115200);		// Output to Serial at 115200 from web console 
	// Debug.println("Start Setup");
	// printf("Start Setup print");
	Serial.begin(115200);
	Serial.println("Start setup");

#if defined(WIFI_LoRa_32_V2)
	// reset the OLED display, e.g. of the heltec_wifi_lora_32 board
	pinMode(RST_OLED, OUTPUT);
	digitalWrite(RST_OLED, LOW);
	delay(50);
	digitalWrite(RST_OLED, HIGH);
#endif

#if defined(ESP8266)
	esp_chipid = std::move(String(ESP.getChipId()));
	esp_mac_id = std::move(String(WiFi.macAddress().c_str()));
	esp_mac_id.replace(":", "");
	esp_mac_id.toLowerCase();
#endif
#if defined(ESP32)
	String esp_chipid = get_chipid();
#endif
	cfg::initNonTrivials(esp_chipid.c_str());
	WiFi.persistent(false);

	debug_outln_info(F("Altruist: " SOFTWARE_VERSION_STR "/"), String(CURRENT_LANG));

	init_config();
	// init_display();
	setupNetworkTime();
	setupEnabledAPIs();
	powerOnTestSensors();
	webserver.setRobonomicsAddress(robonomics.getSs58Address());
	connectWifi(webserver);
	webserver.setup();
	debug_outln_info(F("\nChipId: "), esp_chipid);
	debug_outln_info(get_reset_reason_text());
	twoStageOTAUpdate(deviceStatus);

	sensors_data["service_data"]["robonomics_address"] = robonomics.getSs58Address();
	sensors_data["service_data"]["signal_strength"] = WiFi.RSSI();

	delay(50);

	debug_outln_info(F("Active Sensors count: "), activeSensorsCount);

	Serial.print(F("Sensors: "));
    for (const auto &sensor : deviceStatus.sensor_names) {
        Serial.print(sensor.c_str());
        Serial.print(F(" "));
    }
    Serial.println();

	deviceStatus.last_update_attempt = deviceStatus.time_point_device_start_ms = millis();

	xTaskCreatePinnedToCore(
		sensorAndAPIWorker,  // task function
		"SensorAPIWorker",   // name
		8192,                // stack size
		NULL,                // parameters
		1,                   // priority (>=1 to not be preempted too much)
		NULL,                // task handle (optional)
		0                    // core 0 (ESP32-C3/C6 is single-core anyway)
	);

}

/*****************************************************************
 * And action                                                    *
 *****************************************************************/

void loop(void) {
	webserver.handleClient();
	yield();
}
