#include "message_formatter.h"
#include "../../utils.h"
#include "../../config_manager/config_defaults.h"
#include "../../sensors/sensor_names.h"

#define SCD4X_WARMUP_SEC 360

void formatRobonomicsString(JsonDocument &data, String &datalog_data) {
    datalog_data = "";

    bool has_scd4x = !data[SCD4X_SENSOR_NAME].isNull();
    bool use_bme680_for_temp_hum = !has_scd4x || (millis() / 1000 < SCD4X_WARMUP_SEC);

    for (JsonPair sensor : data.as<JsonObject>())  {
        String sensor_name = sensor.key().c_str();

        JsonObject sensorData = sensor.value().as<JsonObject>();

        for (JsonPair measurement : sensorData) {
            String type = measurement.key().c_str();
            JsonObject measurementData = measurement.value().as<JsonObject>();
            String value;
			if (measurementData["value"].is<uint8_t>()) {
				value = String(measurementData["value"].as<uint8_t>());
			}else if (measurementData["value"].is<float>()) {
				value = String(measurementData["value"].as<float>(), 2);
			} else {
				value = measurementData["value"].as<String>();
			}

            bool is_bme680 = (sensor_name == BME680_SENSOR_NAME);
            bool is_scd4x = (sensor_name == SCD4X_SENSOR_NAME);
            bool skip_temp_hum = (is_bme680 && !use_bme680_for_temp_hum)
                              || (is_scd4x && use_bme680_for_temp_hum);

            if (type == "P1" && cfg::share_pm) datalog_data += "p1:" + value + ",";
            else if (type == "P2" && cfg::share_pm) datalog_data += "p2:" + value + ",";
            else if (type == "noiseMax" && cfg::share_noise) datalog_data += "nm:" + value + ",";
            else if (type == "noiseAvg" && cfg::share_noise) datalog_data += "na:" + value + ",";
            else if (type == "temperature" && cfg::share_temperature && datalog_data.indexOf("t:") == -1
                     && !skip_temp_hum) datalog_data += "t:" + value + ",";
            else if (type == "pressure" && cfg::share_pressure) datalog_data += "p:" + value + ",";
            else if (type == "humidity" && cfg::share_humidity && datalog_data.indexOf("h:") == -1
                     && !skip_temp_hum) datalog_data += "h:" + value + ",";
            else if (type == "radiation") datalog_data += "gc:" + value + ",";
            else if (type == "co2" && cfg::share_co2) datalog_data += "co2:" + value + ",";
            else if (type == "co" && cfg::share_co) datalog_data += "co:" + value + ",";
            else if (type == "o3" && cfg::share_o3) datalog_data += "o3:" + value + ",";
            else if (type == "no2" && cfg::share_no2) datalog_data += "no2:" + value + ",";
            else if (type == "fast_aqi" && cfg::share_fast_aqi) datalog_data += "fa:" + value + ",";
            else if (type == "epa_aqi" && cfg::share_epa_aqi) datalog_data += "ea:" + value + ",";
        }
    }
    if (datalog_data.length() > 0) {
        datalog_data.remove(datalog_data.length() - 1);
    }
    debug_outln_info(F("Datalog data: "), datalog_data);
}

void addTimeAndSign(const String &data, String &signature, Robonomics *robonomics) {
  // Get the local time.
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    debug_outln_verbose(F("Failed to obtain time"));
    return;
  }
  debug_outln_info(F("Local time: "), timeinfo.tm_hour);
  
  // Convert local time to a Unix timestamp.
  time_t timestamp = mktime(&timeinfo);
  String timestampStr = String(timestamp);
  
  // Remove the last two digits from the timestamp string.
  if (timestampStr.length() > 2) {
    timestampStr = timestampStr.substring(0, timestampStr.length() - 2);
  }
  
  debug_outln_info(F("Modified Timestamp: "), timestampStr);

  String messageWithTimestamp = data + ",time:" + timestampStr;

  debug_outln_info(F("Message to sign: "), messageWithTimestamp);

  robonomics->signMessage(messageWithTimestamp, signature);

  debug_outln_info(F("Signature: "), signature);
}
