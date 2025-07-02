#include "pages.h"
#include <ArduinoJson.h>
#include "../../utils.h"
#include "../../config_manager/config_helpers.h"
#include "../../defines.h"
#include "../../sensors/sensor_names.h"

/*****************************************************************
 * Webserver data.json                                           *
 *****************************************************************/
void webserver_data_json(JsonDocument &data, const String &esp_chipid, String &json_content) {
	json_content = "{\"software_version\":\"";
    json_content += SOFTWARE_VERSION_STR;
    json_content += "\", \"sensor_id\":\"";
    json_content += esp_chipid;
    json_content += "\", \"sensordatavalues\":[";
    String signal_strength;
    String meas_id;
	unsigned long age = 0;

    for (JsonPair sensor : data.as<JsonObject>())  {
        String sensor_name = sensor.key().c_str();
        JsonObject sensorData = sensor.value().as<JsonObject>();

        if (sensor_name.startsWith(HTTP_ALTRUIST_SENSOR_NAME)) continue;

        if (sensor_name == "service_data") {
            signal_strength = sensorData["signal_strength"].as<String>();
            json_content += "{\"value_type\":\"signal\",\"value\":\"";
            json_content += signal_strength;
            json_content += "\"},";
            continue;
        }

        for (JsonPair measurement : sensorData) {
            String type = measurement.key().c_str();
            JsonObject measurementData = measurement.value().as<JsonObject>();
            String value = measurementData["value"].as<String>();
            if (sensor_name == "ICS43434") {
                meas_id = "PCBA_" + type;
            } else if (sensor_name == "RadSens") {
                meas_id = "GC";
            } else {
                meas_id = sensor_name + "_" + type;
            }

            json_content += "{\"value_type\":\"";
            json_content += meas_id;
            json_content += "\",\"value\":\"";
            json_content += value;
            json_content += "\"},";
        }
    }
    json_content.remove(json_content.length() - 1);
    json_content += "]}";
}