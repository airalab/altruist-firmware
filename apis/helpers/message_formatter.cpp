#include "message_formatter.h"
#include "../../utils.h"

void formatRobonomicsString(JsonDocument &data, String &datalog_data) {
    datalog_data = "";
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

            if (type == "P1") datalog_data += "p1:" + value + ",";
            else if (type == "P2") datalog_data += "p2:" + value + ",";
            else if (type == "noiseMax") datalog_data += "nm:" + value + ",";
            else if (type == "noiseAvg") datalog_data += "na:" + value + ",";
            else if (type == "temperature" && datalog_data.indexOf("t:") == -1) datalog_data += "t:" + value + ",";
            else if (type == "pressure") datalog_data += "p:" + value + ",";
            else if (type == "humidity" && datalog_data.indexOf("h:") == -1) datalog_data += "h:" + value + ",";
            else if (type == "radiation") datalog_data += "gc:" + value + ",";
            else if (type == "co2") datalog_data += "co:" + value + ","; 
        }
    }
    datalog_data.remove(datalog_data.length() - 1);
    debug_outln_info(F("Datalog data: "), datalog_data);
}

void addTimeAndSign(const String &data, String &signature, Robonomics *robonomics) {
  // Get the local time.
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    debug_outln_error(F("Failed to obtain time"));
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