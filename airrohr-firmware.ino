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
#include <SPIFFS.h>

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

SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
DynamicJsonDocument sensors_data(2048);
device_status_t deviceStatus;
#if defined(USE_SD_CARD)
SDCard sdCardLogger;
#endif

#if defined(ALTRUIST_INSIDE)
DisplayManager displayManager(sensors_data, deviceStatus);
#endif
ButtonManager button_manager;

button_pressed_t btn_press;

#if defined(ALTRUIST_URBAN)
LedControllerUrban leds_controller_urban;
#endif
#if defined(ALTRUIST_INSIDE)
LedControllerInsight leds_controller_insight(sensors_data);
#endif

SensorWebServer webserver(sensors_data, deviceStatus, mutex);

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
	debug_outln_info(F("Setup time, timezone: "), cfg::timezone);
	static char ntpServer1[18], ntpServer2[18];
	strcpy_P(ntpServer1, NTP_SERVER_1);
	strcpy_P(ntpServer2, NTP_SERVER_2);

	configTzTime(cfg::timezone, ntpServer1, ntpServer2);
}

void fetchSensors() {
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
#if defined(USE_SD_CARD)
				deviceStatus.sd_card_connected = sdCardLogger.checkInserted();
				if (deviceStatus.sd_card_connected && activeSensors[i]->jsonUpdated()) {
					sdCardLogger.logData(activeSensors[i]->sensor_name, sensors_data);
				}
#endif
			}
		}
}

void sensorAndAPIWorker(void *pvParameters) {
	int reconnected = 0;
	for (;;) {  // infinite loop
		fetchSensors();

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
			bool senders_ok = true;
			for (const auto& [api_name, status] : deviceStatus.apis_status) {
				Serial.print(F("API Name: "));
				Serial.println(api_name.c_str());
				Serial.print(F("  Count Sends: "));
				Serial.println(status.count_sends);
				Serial.print(F("  Last Send Time: "));
				Serial.println(ctime(&status.last_send_time));
				Serial.print(F("  Is OK: "));
				Serial.println(status.is_ok ? F("Yes") : F("No"));
				senders_ok = senders_ok && status.is_ok;
			}

			sensors_data.shrinkToFit();
#ifdef ALTRUIST_URBAN
			if (senders_ok) {
				leds_controller_urban.setMode(LedMode::BLINK_GREEN);
			} else {
				leds_controller_urban.setMode(LedMode::BLINK_RED);
			}
#endif
			}
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);  // yield to other tasks, run ~10x/sec
	}
}

void ledsWorker(void *pvParameters) {
	for (;;) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
#ifdef ALTRUIST_URBAN
		leds_controller_urban.process();
#endif
#ifdef ALTRUIST_INSIDE
		leds_controller_insight.process();
#endif
	}
}

void buttonsWorker(void *pvParameters) {
	for (;;) {
		button_pressed_t res = button_manager.process();
		vTaskDelay(10 / portTICK_PERIOD_MS);
		if (res.pressed) {
			btn_press.button_num = res.button_num;
			btn_press.press_type = res.press_type;
			btn_press.double_long = res.double_long;
			btn_press.second_button_num = res.second_button_num;
			btn_press.pressed = true;
#ifdef ALTRUIST_URBAN
			if (btn_press.press_type == PressType::LONG) {
				removeWiFiCredentials();
				esp_restart();
			}
#endif
#ifdef ALTRUIST_INSIDE
			if (btn_press.double_long) {
				debug_outln_info(F("Get double long press, reset wifi"));
				removeWiFiCredentials();
				btn_press.pressed = false;
				displayManager.setScreen(ScreenPage::LOGO);
				displayManager.process(btn_press);
				delay(10000);
				esp_restart();
			}
#endif
		}
	}
}


void setup(void) {
	delay(300);
	// Debug.begin(115200);		// Output to Serial at 115200 from web console 
	// Debug.println("Start Setup");
	// printf("Start Setup print");
	Serial.begin(115200);
	Serial.println("Start setup");

	// If SET button pressed while turn on, reset the configuration
#ifdef ALTRUIST_URBAN
	leds_controller_urban.init();
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
	displayManager.setScreen(ScreenPage::LOADING);
	displayManager.process(btn_press);
#endif

	String esp_chipid = get_chipid();
	cfg::initNonTrivials(esp_chipid.c_str());
	WiFi.persistent(false);

	debug_outln_info(F("Altruist: " SOFTWARE_VERSION_STR "/"), String(CURRENT_LANG));

	init_config();
	setupNetworkTime();
	setupEnabledAPIs();
	webserver.setRobonomicsAddress(robonomics.getSs58Address());
#ifdef ALTRUIST_INSIDE
	displayManager.setRobonomicsAddress(robonomics.getSs58Address());
	if (strcmp(cfg::wlanssid, WLANSSID) != 0) {
		displayManager.setScreen(ScreenPage::CONNECTING);
		displayManager.process(btn_press);
	}
#endif
	if (strcmp(cfg::wlanssid, WLANSSID) == 0 || !connectWifi(webserver)) {
#ifdef ALTRUIST_INSIDE
		displayManager.setScreen(ScreenPage::SETUP);
		displayManager.process(btn_press);
#endif
#ifdef ALTRUIST_URBAN
		leds_controller_urban.setMode(LedMode::BLUE);
		leds_controller_urban.process();
#endif
		wifiConfig(webserver);
	}
#ifdef ALTRUIST_URBAN
		leds_controller_urban.setMode(LedMode::GREEN);
		leds_controller_urban.process();
#endif
	powerOnTestSensors();
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
#if defined(USE_SD_CARD)
	deviceStatus.sd_card_connected = sdCardLogger.begin();
#endif
	fetchSensors();
	deviceStatus.ip_address = WiFi.localIP().toString();

#ifdef ALTRUIST_URBAN
	leds_controller_urban.setMode(LedMode::NONE);
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
	xTaskCreatePinnedToCore(
		buttonsWorker,  // task function
		"ButtonWorker",   // name
		2048,                // stack size
		NULL,                // parameters
		1,                   // priority (>=1 to not be preempted too much)
		NULL,                // task handle (optional)
		0                    // core 0 (ESP32-C3/C6 is single-core anyway)
	);
	xTaskCreatePinnedToCore(
		ledsWorker,  // task function
		"LedsWorker",   // name
		2048,                // stack size
		NULL,                // parameters
		2,                   // priority (>=1 to not be preempted too much)
		NULL,                // task handle (optional)
		0                    // core 0 (ESP32-C3/C6 is single-core anyway)
	);
#ifdef ALTRUIST_INSIDE
	displayManager.setScreen(ScreenPage::MAIN);
#endif
	debug_outln_info(F("Setup finished"));
	// button_controller.init();
}

void loop(void) {
	webserver.handleClient();
#if defined(ALTRUIST_INSIDE)
	displayManager.process(btn_press);
#endif
	yield();
}
