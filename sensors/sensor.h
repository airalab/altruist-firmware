#ifndef SENSOR_H
#define SENSOR_H

#include "Arduino.h"
#include <ArduinoJson.h>
#include "../utils.h"

class Sensor {
protected:
  unsigned long sending_timeout;  // Private variable for sending timeout
  unsigned long timeout;  // Private variable for sensor timeout
  unsigned long last_fetch_time;
  bool _jsonUpdated = false;

  virtual void _fetch(JsonDocument &data) = 0;

  void updateFetchTime() {
    last_fetch_time = millis();
  }

  void addValueToJSON(JsonDocument &data, const String &meas_id, const float value, const String &intl_name, const String &units) {
    // Ensure sensor_name object exists
    // debug_outln_info(F("Meas_id: "), meas_id);
    // debug_outln_info(F("Value: "), value);
    String sensor_name_copy(sensor_name);
    JsonObject sensorObj = data[sensor_name_copy];  
    if (!sensorObj) {
        sensorObj = data.createNestedObject(sensor_name_copy);
    }

    // Ensure measurement object exists
    JsonObject measObj = sensorObj[meas_id];  
    if (!measObj) {
        measObj = sensorObj.createNestedObject(meas_id);
    }

    // Directly update values without removing objects
    measObj[F("value")] = value;
    measObj[F("intl_name")] = intl_name;
    measObj[F("units")] = units;
    _jsonUpdated = true;
  }

  void addValueToJSON(JsonDocument &data, const String &meas_id, const uint8_t &value, const String &intl_name, const String &units) {
    // Ensure sensor_name object exists
    String sensor_name_copy(sensor_name);
    JsonObject sensorObj = data[sensor_name_copy];  
    if (!sensorObj) {
        sensorObj = data.createNestedObject(sensor_name_copy);
    }

    // Ensure measurement object exists
    JsonObject measObj = sensorObj[meas_id];  
    if (!measObj) {
        measObj = sensorObj.createNestedObject(meas_id);
    }

    // Directly update values without removing objects
    measObj[F("value")] = value;
    measObj[F("intl_name")] = intl_name;
    measObj[F("units")] = units;
    _jsonUpdated = true;
  }

  void addValueToJSON(JsonDocument &data, const String &meas_id, const double &value, const String &intl_name, const String &units) {
    // Ensure sensor_name object exists
    String sensor_name_copy(sensor_name);
    JsonObject sensorObj = data[sensor_name_copy];  
    if (!sensorObj) {
        sensorObj = data.createNestedObject(sensor_name_copy);
    }

    // Ensure measurement object exists
    JsonObject measObj = sensorObj[meas_id];  
    if (!measObj) {
        measObj = sensorObj.createNestedObject(meas_id);
    }

    // Directly update values without removing objects
    measObj[F("value")] = value;
    measObj[F("intl_name")] = intl_name;
    measObj[F("units")] = units;
    _jsonUpdated = true;
  }

  void addValueToJSON(JsonDocument &data, const String &meas_id, const String &value, const String &intl_name, const String &units) {
    // Ensure sensor_name object exists
    String sensor_name_copy(sensor_name);
    JsonObject sensorObj = data[sensor_name_copy];  
    if (!sensorObj) {
        sensorObj = data.createNestedObject(sensor_name_copy);
    }

    // Ensure measurement object exists
    JsonObject measObj = sensorObj[meas_id];  
    if (!measObj) {
        measObj = sensorObj.createNestedObject(meas_id);
    }

    // Directly update values without removing objects
    measObj[F("value")] = value;
    measObj[F("intl_name")] = intl_name;
    measObj[F("units")] = units;
    _jsonUpdated = true;
  }

public:
  // Constructor with a default timeout value (e.g., 1000 milliseconds)
  Sensor(unsigned long sending_timeout = 1000UL) : sending_timeout(sending_timeout) {}

  virtual ~Sensor() {}

  const char* sensor_name;

  // Pure virtual function to attempt sensor initialization.
  virtual bool begin() = 0;

  void fetch(JsonDocument &data) {
    _fetch(data);
    updateFetchTime();
  };

  // Optional getter for the timeout.
  bool isTimeToFetch() const {
    return (millis() - last_fetch_time > timeout);
  }

  bool jsonUpdated() {
    if (_jsonUpdated) {
      _jsonUpdated = false;
      return true;
    } else {
      return false;
    }
  }
};

#endif  // SENSOR_H