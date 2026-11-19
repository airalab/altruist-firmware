// Exercise the actual Sensor fetch/metadata adapter with a deterministic clock
// and acquisition outcomes. Only hardware/runtime services are stubbed.
#include "sensors/sensor.h"
#include <cstdlib>
#include <iostream>
uint64_t fakeNow = 0;
FakeSerial Serial;
bool refuseStringReserve = false;
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
  void probe(bool active) {
    uart_v2::Probe p = debugProbe;
    p.expectedBytes = 8192;
    p.bytes = active ? 8192 : 0;
    p.hasSignal = active;
    p.signalMin = -10; p.signalMax = 10;
    p.record(fakeNow, active, active, active ? "activity" : "read_failed");
    debugSetProbe(p);
  }
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

static void emit(const char *scenario, Sensor &sensor) {
  Serial.output.clear();
  const size_t writes = Serial.writes;
  sensor.debugSnapshotIfDue();
  CHECK(Serial.writes == writes + 1);
  CHECK(Serial.output.back() == '\n');
  CHECK(Serial.output.find('\n') == Serial.output.size() - 1);
  std::cout << scenario << '\t' << Serial.output;
}

int main() {
  DynamicJsonDocument data(4096);
  Reading lost("BME280");
  lost.debugPrepare();
  Serial.limits = {8, 0};
  lost.debugSnapshotIfDue();
  CHECK(Serial.output == "{\"BME280");
  Serial.output.clear();
  Reading other("SCD4x");
  other.debugPrepare();
  other.debugSnapshotIfDue();
  CHECK(Serial.output.rfind("\n{", 0) == 0 && Serial.output.back() == '\n');
  Serial.output.clear();
  refuseStringReserve = true;
  Reading outOfMemory("BME280");
  outOfMemory.debugPrepare();
  outOfMemory.debugSnapshotIfDue();
  CHECK(Serial.output == "{\"_observations\":{\"version\":2,\"error\":\"snapshot_overflow\"}}\n");
  refuseStringReserve = false;

  Reading sensor("BME280");
  sensor.debugPrepare();
  emit("adapter_bme", sensor);
  CHECK(!data.containsKey("_observations"));
  fakeNow = 1000;
  sensor.fetch(data);
  CHECK(sensor.cycles() == 1 && sensor.jsonUpdated());
  emit("adapter_bme", sensor);
  CHECK(!sensor.jsonUpdated());  // UART publication did not consume API state.
  fakeNow = 31000;
  const size_t count = Serial.writes;
  sensor.debugSnapshotIfDue();
  CHECK(Serial.writes == count);  // cached fields do not create a heartbeat.
  fakeNow = 301000;
  sensor.fetch(data);
  emit("adapter_bme", sensor);  // equal numbers advance all identities once.
  fakeNow = 601000;
  sensor.failed = true;
  sensor.fetch(data);
  emit("adapter_bme", sensor);
  fakeNow = 901000;
  sensor.failed = false;
  sensor.fetch(data);
  // Prove that publication leaves pending API data available.
  emit("adapter_bme", sensor);
  CHECK(sensor.jsonUpdated());
  CHECK(!data.containsKey("_observations"));
  fakeNow = 1201000;
  sensor.partial = true;
  sensor.fetch(data);
  emit("adapter_bme", sensor);  // incomplete cycle cannot freshen one field.

  Reading absent("BME280");
  absent.debugPrepare();
  absent.initFail();
  CHECK(absent.cycles() == 0);
  emit("adapter_init", absent);

  PMWindow pm;
  pm.debugPrepare();
  emit("adapter_sds", pm);
  pm.fetch(data);
  pm.fetch(data);
  CHECK(pm.cycles() == 0);  // UART polls do not complete PM windows.
  Reading mic("ICS43434");
  mic.debugPrepare();
  mic.debugSetSdsAvailable(true);
  emit("adapter_pause", mic);
  fakeNow += 1000;
  Sensor::debugSdsTransition(true);
  emit("adapter_pause", mic);
  fakeNow += 20000;
  Sensor::debugSdsTransition(false);
  emit("adapter_pause", mic);
  pm.ready = true;
  pm.fetch(data);
  emit("adapter_sds", pm);
  CHECK(pm.cycles() == 1);
  pm.fetch(data);
  CHECK(pm.cycles() == 1);

  // Several error/recovery outcomes inside the state throttle must survive
  // one publication, including failures before any successful result.
  fakeNow = 2000000;
  Reading burst("BME280");
  burst.debugPrepare();
  emit("adapter_burst", burst);
  for (unsigned i = 1; i <= 3; ++i) {
    fakeNow = 2000000 + i * 100;
    burst.failed = i != 3;
    burst.fetch(data);
    const size_t writes = Serial.writes;
    burst.debugSnapshotIfDue();
    CHECK(Serial.writes == writes);
  }
  fakeNow = 2001000;
  emit("adapter_burst", burst);

  // Exercise the actual adapter's acquisition-only trigger, not just due().
  fakeNow = 3000000;
  Reading probeMic("ICS43434");
  probeMic.debugPrepare();
  emit("adapter_probe", probeMic);
  for (unsigned tick = 1; tick <= 1210; ++tick) {
    fakeNow = 3000000 + tick * 100;
    probeMic.probe(true);
    if (tick == 10 || (tick > 10 && (tick - 10) % 300 == 0))
      emit("adapter_probe", probeMic);
    else {
      const size_t writes = Serial.writes;
      probeMic.debugSnapshotIfDue();
      CHECK(Serial.writes == writes);
    }
  }
  // No new attempts: even a long pause must not repeat cached probes.
  fakeNow += 300000;
  const size_t writes = Serial.writes;
  probeMic.debugSnapshotIfDue();
  CHECK(Serial.writes == writes);
  // Error then recovery inside 1s survives as counters/latest result.
  probeMic.probe(false);
  fakeNow += 100;
  probeMic.probe(true);
  emit("adapter_probe", probeMic);
}
