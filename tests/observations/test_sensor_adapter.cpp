// Exercise the actual Sensor fetch/metadata adapter with a deterministic clock
// and acquisition outcomes. Only hardware/runtime services are stubbed.
#include "sensors/sensor.h"
#include <cstdlib>
#include <iostream>
uint64_t fakeNow = 0;
FakeSerial Serial;
#define CHECK(condition) do { if (!(condition)) { \
  std::cerr << __LINE__ << ": " << #condition << '\n'; std::exit(1); \
} } while (false)

class Reading : public Sensor {
public:
  bool failed = false, partial = false;
  explicit Reading(const char *name) : Sensor(300000) { sensor_name = name; timeout = 300000; }
  bool begin() override { return true; }
  void _fetch(JsonDocument &data) override {
    if (failed) { debugReadingError("injected_read_failure"); return; }
    addValueToJSON(data, "temperature", 24.0, "Temperature", "°C");
    if (partial) return;
    addValueToJSON(data, "humidity", 45.0, "Humidity", "%");
    addValueToJSON(data, "pressure", 1013.0, "Pressure", "hPa");
  }
  uint64_t cycles() const { return debugTelemetry.cycle; }
  void initFail() { debugReadingError("injected_init_failure"); }
};

class PMWindow : public Sensor {
public:
  bool ready = false;
  PMWindow() : Sensor(300000) { sensor_name = "SDS"; timeout = 1000; }
  bool begin() override { return true; }
  void _fetch(JsonDocument &data) override {
    if (!ready) return;
    addValueToJSON(data, "P1", 10.0, "PM10", "µg/m³");
    addValueToJSON(data, "P2", 5.0, "PM2.5", "µg/m³");
    debugCompleteCycle(true);
    ready = false;
  }
  uint64_t cycles() const { return debugTelemetry.cycle; }
};

static void emit(const char *scenario, Sensor &sensor, JsonDocument &data) {
  Serial.output.clear();
  const size_t writes = Serial.writes;
  sensor.debugSnapshotIfDue(data);
  CHECK(Serial.writes == writes + 1);
  CHECK(Serial.output.back() == '\n');
  CHECK(Serial.output.find('\n') == Serial.output.size() - 1);
  std::cout << scenario << '\t' << Serial.output;
}

int main() {
  DynamicJsonDocument data(4096);
  Reading sensor("BME280");
  sensor.debugPrepare();
  emit("adapter_bme", sensor, data);
  CHECK(!data.containsKey("_observations"));
  fakeNow = 1000;
  sensor.fetch(data);
  CHECK(sensor.cycles() == 1 && sensor.jsonUpdated());
  emit("adapter_bme", sensor, data);
  CHECK(!sensor.jsonUpdated());  // UART publication did not consume API state.
  fakeNow = 31000;
  const size_t count = Serial.writes;
  sensor.debugSnapshotIfDue(data);
  CHECK(Serial.writes == count);  // cached fields do not create a heartbeat.
  fakeNow = 301000;
  sensor.fetch(data);
  emit("adapter_bme", sensor, data);  // equal numbers advance all identities once.
  fakeNow = 601000;
  sensor.failed = true;
  sensor.fetch(data);
  emit("adapter_bme", sensor, data);
  fakeNow = 901000;
  sensor.failed = false;
  sensor.fetch(data);
  // Prove that publication leaves pending API data available.
  emit("adapter_bme", sensor, data);
  CHECK(sensor.jsonUpdated());
  CHECK(!data.containsKey("_observations"));
  fakeNow = 1201000;
  sensor.partial = true;
  sensor.fetch(data);
  emit("adapter_bme", sensor, data);  // incomplete cycle cannot freshen one field.

  Reading absent("BME280");
  absent.debugPrepare();
  absent.initFail();
  CHECK(absent.cycles() == 0);
  emit("adapter_init", absent, data);

  PMWindow pm;
  pm.debugPrepare();
  emit("adapter_sds", pm, data);
  pm.fetch(data);
  pm.fetch(data);
  CHECK(pm.cycles() == 0);  // UART polls do not complete PM windows.
  Reading mic("ICS43434");
  mic.debugPrepare();
  mic.debugSetSdsAvailable(true);
  emit("adapter_pause", mic, data);
  fakeNow += 1000;
  Sensor::debugSdsTransition(true);
  emit("adapter_pause", mic, data);
  fakeNow += 20000;
  Sensor::debugSdsTransition(false);
  emit("adapter_pause", mic, data);
  pm.ready = true;
  pm.fetch(data);
  emit("adapter_sds", pm, data);
  CHECK(pm.cycles() == 1);
  pm.fetch(data);
  CHECK(pm.cycles() == 1);

}
