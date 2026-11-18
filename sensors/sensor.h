#ifndef SENSOR_H
#define SENSOR_H

#include "Arduino.h"
#include <ArduinoJson.h>
#include "../utils.h"
#if defined(ALTRUIST_BUILD_DEBUG)
#include "uart_v2_json.h"
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
  uart_v2::Telemetry debugTelemetry;
  uart_v2::Metric debugMetrics[16];
  size_t debugMetricCount = 0;
  uint16_t debugWritten = 0;
  double debugValues[16] = {};
  uart_v2::Kind debugKind = uart_v2::Kind::Bme280;
  uart_v2::Noise debugNoise;
  uart_v2::Probe debugProbe;
  uint64_t debugPublishedAttempts = 0;
  const char *debugFetchError = nullptr;
  bool debugPrepared = false, debugFetching = false, debugActiveSds = false;

  static uint64_t debugNowMs() {
    return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
  }
  static Sensor *&debugNoiseObserver() {
    static Sensor *observer = nullptr;
    return observer;
  }
  static uint64_t &debugSdsCycle() { static uint64_t cycle = 0; return cycle; }
  static bool &debugSdsRunning() { static bool running = false; return running; }
  static const String &debugBootId() {
    static const String id = String(esp_random(), HEX) + "-" + String(esp_random(), HEX);
    return id;
  }
  void debugAddMetric(const char *name, const char *units, uint8_t channel = 0) {
    debugMetrics[debugMetricCount++] = uart_v2::Metric(name, units, channel);
  }
  void debugCompleteCycle(bool ok, const char *reason = "read_failed") {
    if (!debugPrepared) return;
    for (size_t i = 0; i < debugMetricCount; ++i) {
      if (debugMetrics[i].channel != 0) continue;
      if (!(debugWritten & (1u << i)) || !isfinite(debugValues[i])) ok = false;
    }
    const uint64_t now = debugNowMs();
    debugTelemetry.complete(now, 1, ok ? 0 : 1, reason);
    if (ok) {
      for (size_t i = 0; i < debugMetricCount; ++i)
        if (debugMetrics[i].channel == 0) debugMetrics[i].update(debugValues[i], now);
    }
  }
  void debugSetProbe(const uart_v2::Probe &probe) {
    if (strcmp(debugProbe.result, probe.result) != 0) debugTelemetry.statePending = true;
    debugProbe = probe;
  }
#endif

  void debugReadingError(const char *reason) {
#if defined(ALTRUIST_BUILD_DEBUG)
    if (!debugPrepared) return;
    if (debugFetching) debugFetchError = reason;
    else if (debugKind != uart_v2::Kind::Noise) debugTelemetry.initError(reason);
#else
    (void)reason;
#endif
  }

  void debugMetricError(const String &name, const char *reason) {
    (void)name;
    debugReadingError(reason);
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
    if (debugPrepared && debugKind != uart_v2::Kind::Noise) {
      for (size_t i = 0; i < debugMetricCount; ++i) {
        if (meas_id == debugMetrics[i].name && debugMetrics[i].channel == 0) {
          debugValues[i] = measObj[F("value")].as<double>();
          debugWritten |= 1u << i;
        }
      }
    }
#endif
  }

public:
  // Constructor with a default timeout value (e.g., 1000 milliseconds)
  Sensor(unsigned long sending_timeout = 1000UL) : sending_timeout(sending_timeout) {}

  virtual ~Sensor() {
#if defined(ALTRUIST_BUILD_DEBUG)
    if (debugNoiseObserver() == this) debugNoiseObserver() = nullptr;
#endif
  }

  const char* sensor_name = "";

  // Pure virtual function to attempt sensor initialization.
  virtual bool begin() = 0;

  void fetch(JsonDocument &data) {
#if defined(ALTRUIST_BUILD_DEBUG)
    debugWritten = 0;
    debugFetchError = nullptr;
    debugFetching = true;
#endif
    _fetch(data);
#if defined(ALTRUIST_BUILD_DEBUG)
    if (debugPrepared && debugKind != uart_v2::Kind::Sds && debugKind != uart_v2::Kind::Noise)
      debugCompleteCycle(debugFetchError == nullptr, debugFetchError ? debugFetchError : "invalid_numeric");
    debugFetching = false;
#endif
    updateFetchTime();
  };

  bool debugSupported() const {
    return strcmp(sensor_name, "BME280") == 0 || strcmp(sensor_name, "BMP280") == 0 ||
      strcmp(sensor_name, "BME680") == 0 || strcmp(sensor_name, "SCD4x") == 0 ||
      strcmp(sensor_name, "SDS") == 0 || strcmp(sensor_name, "ICS43434") == 0;
  }

  void debugPrepare() {
#if defined(ALTRUIST_BUILD_DEBUG)
    if (!debugSupported()) return;
    if (debugPrepared && strcmp(sensor_name, uart_v2::sensorName(debugKind)) == 0) return;
    using uart_v2::Kind;
    const String name(sensor_name);
    debugKind = name == "BMP280" ? Kind::Bmp280 : name == "BME680" ? Kind::Bme680 :
      name == "SCD4x" ? Kind::Scd4x : name == "SDS" ? Kind::Sds :
      name == "ICS43434" ? Kind::Noise : Kind::Bme280;
    debugMetricCount = 0;
    if (debugKind == Kind::Noise) {
      debugAddMetric("noiseAvg", "dBA"); debugAddMetric("noiseMax", "dBA");
      debugNoise = uart_v2::Noise(debugNowMs());
      debugNoiseObserver() = this;
      debugTelemetry.setState(0, uart_v2::State::Waiting, "driver_unverified");
    } else if (debugKind == Kind::Sds) {
      debugAddMetric("P1", "µg/m³"); debugAddMetric("P2", "µg/m³");
    } else {
      debugAddMetric("temperature", "°C");
      debugAddMetric("humidity", "%");
      if (debugKind == Kind::Scd4x) debugAddMetric("co2", "ppm");
      else {
        if (debugKind == Kind::Bmp280) --debugMetricCount;
        debugAddMetric("pressure", debugKind == Kind::Bme680 ? "Pa" : "hPa");
      }
      if (debugKind == Kind::Bme680) {
        debugTelemetry.channelCount = 2;
        debugTelemetry.setState(1, uart_v2::State::Waiting, "driver_unverified");
        debugAddMetric("gas_resistance", "Ohm", 1); debugAddMetric("gas_baseline", "Ohm", 1);
        debugAddMetric("voc_ppb", "ppb", 1); debugAddMetric("voc_index", "a.u.", 1);
        debugAddMetric("voc_level", "", 1); debugAddMetric("voc_spikes_today", "", 1);
      }
    }
    debugPrepared = true;
#endif
  }

  void debugSetSdsAvailable(bool available) {
#if defined(ALTRUIST_BUILD_DEBUG)
    debugActiveSds = available;
    if (debugKind == uart_v2::Kind::Noise) debugNoise.pauseBudget = available ? 20000 : 0;
#else
    (void)available;
#endif
  }

  static void debugSdsTransition(bool running) {
#if defined(ALTRUIST_BUILD_DEBUG)
    if (running == debugSdsRunning()) return;
    debugSdsRunning() = running;
    if (running) ++debugSdsCycle();
    auto *observer = debugNoiseObserver();
    if (observer && observer->debugNoise.pauseBudget &&
        observer->debugNoise.setPause(debugNowMs(), debugSdsCycle(), running))
      observer->debugTelemetry.statePending = true;
#else
    (void)running;
#endif
  }

  void debugSnapshotIfDue(JsonDocument &data) {
#if defined(ALTRUIST_BUILD_DEBUG)
    (void)data;
    if (!debugPrepared) return;
    const uint64_t now = debugNowMs();
    const bool noise = debugKind == uart_v2::Kind::Noise;
    const bool limited = noise || debugKind == uart_v2::Kind::Bme680;
    if (noise) debugNoise.advance(now);
    const bool probeDue = limited && debugProbe.attempts != debugPublishedAttempts &&
      now - debugTelemetry.lastPrint >= 30000;
    if (!debugTelemetry.due(now, noise) && !probeDue) return;
    uart_v2::Settings settings(debugKind, timeout, noise ? 0 :
      debugKind == uart_v2::Kind::Sds ? sending_timeout : timeout,
      debugKind == uart_v2::Kind::Bme680, debugActiveSds);
    DynamicJsonDocument snapshot(16384);
    const String device = get_chipid();
#if defined(ALTRUIST_INSIGHT)
    const char *model = "insight";
#else
    const char *model = "urban";
#endif
    uart_v2::formSnapshot(snapshot, debugTelemetry, settings, debugMetrics,
      debugMetricCount, device.c_str(), debugBootId().c_str(), model, now,
      noise ? &debugNoise : nullptr, limited ? &debugProbe : nullptr);
    debugPublishedAttempts = debugProbe.attempts;
    String line;
    serializeJson(snapshot, line);
    line += '\n';
    Serial.write(reinterpret_cast<const uint8_t *>(line.c_str()), line.length());
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
