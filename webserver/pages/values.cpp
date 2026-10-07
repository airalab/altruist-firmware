#include "pages.h"
#include <WiFi.h>
#include "../utils.h"
#include "../../utils.h"
#include "../../intl.h"
#include "../../defines.h"
#include "../../sensors/sensor_names.h"
#include "../../config_manager/config_helpers.h"
#include "../html-content.h"
#include <ArduinoJson.h>

namespace {

const __FlashStringHelper* data_section_label_for(const String& sensor_name) {
	if (sensor_name == SDS_SENSOR_NAME) {
		return FPSTR(INTL_DATA_SECTION_SDS);
	}
	if (sensor_name == SCD4X_SENSOR_NAME) {
		return FPSTR(INTL_DATA_SECTION_SCD);
	}
	if (sensor_name == BME_SENSOR_NAME || sensor_name == BME680_SENSOR_NAME || sensor_name == BMP_SENSOR_NAME) {
		return FPSTR(INTL_DATA_SECTION_BME);
	}
	if (sensor_name == ATRUIST_URBAN_SENSOR) {
		return FPSTR(INTL_DATA_SECTION_URBAN);
	}
	return nullptr;
}

#if defined(ALTRUIST_INSIDE)
bool is_urban_values_sensor(const String& sensor_name) {
	return sensor_name == ATRUIST_URBAN_SENSOR;
}

bool should_show_urban_values() {
	// Standalone Insight has no Urban pair — don't show outdoor Urban block in readings.
	return !cfg::standalone;
}
#endif

void render_sensor_section(
	String &page_content,
	WebServer &server,
	const String& sensor_name,
	JsonObject sensorData) {
	const __FlashStringHelper* section_label = data_section_label_for(sensor_name);
	if (section_label) {
		add_data_section_start(page_content, section_label);
	} else {
		add_data_section_start(page_content, sensor_name);
	}

	bool grid_open = false;
	for (JsonPair measurement : sensorData) {
		String type = measurement.key().c_str();
		JsonObject measurementData = measurement.value().as<JsonObject>();
		String value;
		if (measurementData["value"].is<uint8_t>()) {
			value = String(measurementData["value"].as<uint8_t>());
		} else if (measurementData["value"].is<float>()) {
			float floatValue = measurementData["value"].as<float>();
			if (type == "pressure") {
				floatValue = floatValue * 0.750062 * 0.01;
				value = String(floatValue, 0);
			} else if (type == "gas_resistance" || type == "gas_baseline" || type == "voc_spikes_today") {
				value = String((uint32_t)(floatValue + 0.5f));
			} else {
				value = String(floatValue, 2);
			}
		} else {
			value = measurementData["value"].as<String>();
		}
		String intl_param = measurementData["intl_name"].as<String>();
		String units;
		if (type == "pressure") {
			units = "mm Hg";
		} else {
			units = measurementData["units"].as<String>();
		}

		if (!grid_open) {
			add_reading_metrics_grid_start(page_content);
			grid_open = true;
		}
		add_reading_metric_card(page_content, intl_param, value, units.c_str());
	}
	if (grid_open) {
		add_reading_metrics_grid_end(page_content);
	}
	add_data_section_end(page_content);
	web_page_flush_chunk(page_content, &server);
}

} // namespace

/*****************************************************************
 * Webserver root: show latest values                            *
 *****************************************************************/
void webserver_values(JsonDocument &data, String &page_content, WebServer &server, bool hub_embed) {
	debug_outln_info(F("ws: values ..."));
	int8_t signal_strength = 0;

	if (!hub_embed) {
		append_app_page_body_start(page_content, F(INTL_PAGE_READINGS_INTRO));
	}
	page_content += F("<div class='data-sheet data-sheet--readings'>");

	JsonObject readings = data.as<JsonObject>();

	for (JsonPair sensor : readings) {
		String sensor_name = sensor.key().c_str();
		JsonObject sensorData = sensor.value().as<JsonObject>();

		if (sensor_name == "service_data") {
			signal_strength = sensorData["signal_strength"].as<int8_t>();
			debug_outln_info(F("Signal strength: "), signal_strength);
			continue;
		}

#if defined(ALTRUIST_INSIDE)
		if (is_urban_values_sensor(sensor_name)) {
			continue;
		}
#endif

		render_sensor_section(page_content, server, sensor_name, sensorData);
	}

#if defined(ALTRUIST_INSIDE)
	if (should_show_urban_values() && readings.containsKey(ATRUIST_URBAN_SENSOR)) {
		render_sensor_section(
			page_content,
			server,
			String(ATRUIST_URBAN_SENSOR),
			readings[ATRUIST_URBAN_SENSOR].as<JsonObject>());
	}
#endif

	const int signal_quality = calcWiFiSignalQuality(signal_strength);

	add_data_section_start(page_content, FPSTR(INTL_DATA_SECTION_NETWORK));
	add_data_block_intro(page_content, F(INTL_READINGS_SECTION_NETWORK_INTRO));
	add_reading_metrics_grid_start(page_content);
	add_reading_metric_card(page_content, FPSTR(INTL_SIGNAL_STRENGTH), String(signal_strength), "dBm");
	add_reading_metric_card(page_content, FPSTR(INTL_SIGNAL_QUALITY), String(signal_quality), "%");
	add_reading_metrics_grid_end(page_content);
	add_data_section_end(page_content);

	page_content += F("</div>");
	if (!hub_embed) {
		append_app_page_body_end(page_content);
	}
}
