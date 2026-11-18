#ifndef SENSOR_H
#define SENSOR_H

#include "Arduino.h"
#include <ArduinoJson.h>
#include "../utils.h"
#if defined(ALTRUIST_BUILD_DEBUG)
#include <esp_timer.h>
#include <esp_system.h>
#endif

class Sensor {
protected:
  unsigned long sending_timeout;  // Private variable for sending timeout
  unsigned long timeout;  // Private variable for sensor timeout
  unsigned long last_fetch_time;
  bool _jsonUpdated = false;

  virtual void _fetch(JsonDocument &data) = 0;

#if defined(ALTRUIST_BUILD_DEBUG)
  struct DebugMetric {
    String name;
    uint64_t sequence = 0;
    uint64_t measuredMs = 0;
    const char *state = "never";
  };
  DebugMetric debugMetrics[16];
  size_t debugMetricCount = 0;
  uint64_t debugSnapshotSequence = 0;
  uint64_t debugLastPrintMs = 0;
  uint32_t debugErrors = 0;
  const char *debugState = "never";
  const char *debugReason = "none";
  bool debugChanged = false;

  static void debugUnsigned(JsonObject object, const char *key, uint64_t number) {
    // Keep 64-bit clocks exact without changing global ArduinoJson configuration.
    char text[24];
    snprintf(text, sizeof(text), "%llu", static_cast<unsigned long long>(number));
    object[key] = text;
  }

  static uint64_t debugNowMs() {
    return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
  }

  static const String &debugBootId() {
    // One token per boot, shared by all sensor instances, including late attach.
    static const String id = String(esp_random(), HEX) + "-" + String(esp_random(), HEX);
    return id;
  }

  void debugMetricUpdated(const String &name) {
    size_t i = 0;
    for (; i < debugMetricCount; ++i) {
      if (debugMetrics[i].name == name) break;
    }
    if (i == debugMetricCount) {
      if (i == 16) {
        debugReadingError("metric_capacity");
        return;
      }
      debugMetrics[i].name = name;
      ++debugMetricCount;
    }
    ++debugMetrics[i].sequence;
    debugMetrics[i].measuredMs = debugNowMs();
    debugMetrics[i].state = "ok";
    debugState = "ok";
    debugReason = "none";
  }
#endif

  void debugReadingError(const char *reason) {
#if defined(ALTRUIST_BUILD_DEBUG)
    ++debugErrors;
    debugChanged = true;
    debugState = "error";
    debugReason = reason;
    for (size_t i = 0; i < debugMetricCount; ++i) debugMetrics[i].state = "error";
#else
    (void)reason;
#endif
  }

  void debugMetricError(const String &name, const char *reason) {
#if defined(ALTRUIST_BUILD_DEBUG)
    for (size_t i = 0; i < debugMetricCount; ++i) {
      if (debugMetrics[i].name == name) debugMetrics[i].state = "error";
    }
    ++debugErrors;
    debugChanged = true;
    debugReason = reason;
#else
    (void)name;
    (void)reason;
#endif
  }

  void printDebugSnapshot(JsonDocument &data) {
#if defined(ALTRUIST_BUILD_DEBUG)
    // This document is UART-only: metadata must never enter API, SD or uplink.
    DynamicJsonDocument snapshot(8192);
    snapshot.createNestedObject(sensor_name);
    if (data[sensor_name].is<JsonObject>()) snapshot[sensor_name].set(data[sensor_name]);
    JsonObject meta = snapshot.createNestedObject("_observations");
    meta["version"] = 1;
    meta["device_id"] = get_chipid();
    meta["boot_id"] = debugBootId();
#if defined(ALTRUIST_INSIGHT)
    meta["model"] = "insight";
#else
    meta["model"] = "urban";
#endif
    meta["sensor"] = sensor_name;
    debugUnsigned(meta, "snapshot_seq", ++debugSnapshotSequence);
    debugUnsigned(meta, "time_ms", debugNowMs());
    meta["state"] = debugState;
    meta["errors"] = debugErrors;
    meta["reason"] = debugReason;
    for (size_t i = 0; i < debugMetricCount; ++i) {
      JsonObject metric = snapshot[sensor_name][debugMetrics[i].name];
      if (metric.isNull()) continue;
      JsonObject observation = metric.createNestedObject("observation");
      debugUnsigned(observation, "seq", debugMetrics[i].sequence);
      debugUnsigned(observation, "measured_ms", debugMetrics[i].measuredMs);
      observation["state"] = debugMetrics[i].state;
    }
    if (snapshot.overflowed() || measureJson(snapshot) > 8192) {
      Serial.println(F("{\"_observations\":{\"version\":1,\"error\":\"snapshot_overflow\"}}"));
    } else {
      String line;
      serializeJson(snapshot, line);
      line += '\n';
      // One write prevents other logging tasks from splitting a JSON record.
      Serial.write(reinterpret_cast<const uint8_t *>(line.c_str()), line.length());
    }
    debugLastPrintMs = debugNowMs();
    debugChanged = false;
#else
    (void)data;
#endif
  }


  void updateFetchTime() {
    last_fetch_time = millis();
  }

  // ArduinoJson 6 does not free pool memory when strings are replaced. Set
  // intl_name/units only on first create; afterwards only update numeric/string value.
  template <typename T>
  void addValueToJSON(JsonDocument &data, const String &meas_id, const T &value,
		      const String &intl_name, const String &units) {
    String sensor_name_copy(sensor_name);
    JsonObject sensorObj = data[sensor_name_copy];
    if (!sensorObj) {
      sensorObj = data.createNestedObject(sensor_name_copy);
    }

    JsonObject measObj = sensorObj[meas_id];
    if (!measObj) {
      measObj = sensorObj.createNestedObject(meas_id);
      if (!measObj) {
	return;
      }
      measObj[F("intl_name")] = intl_name;
      measObj[F("units")] = units;
    }

    measObj[F("value")] = value;
    _jsonUpdated = true;
#if defined(ALTRUIST_BUILD_DEBUG)
    if (measObj[F("value")].is<double>()) {
      if (isfinite(measObj[F("value")].as<double>())) debugMetricUpdated(meas_id);
      else debugMetricError(meas_id, "invalid_numeric");
    }
#endif
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

  void debugSnapshotIfDue(JsonDocument &data) {
#if defined(ALTRUIST_BUILD_DEBUG)
    const String name(sensor_name);
    if (name != "BME280" && name != "BMP280" && name != "BME680" &&
        name != "SCD4x" && name != "SDS" && name != "ICS43434") return;
    if (debugSnapshotSequence == 0 || debugChanged || debugNowMs() - debugLastPrintMs >= 30000) {
      printDebugSnapshot(data);
    }
#else
    (void)data;
#endif
  }

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
