#include "sensors/uart_v2_json.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace uart_v2;
#define CHECK(condition) do { if (!(condition)) { \
  std::cerr << __LINE__ << ": " << #condition << '\n'; std::exit(1); \
} } while (false)

static std::vector<Metric> metrics(Kind kind) {
  switch (kind) {
    case Kind::Bme280: return {{"temperature", "°C"}, {"humidity", "%"}, {"pressure", "hPa"}};
    case Kind::Bmp280: return {{"temperature", "°C"}, {"pressure", "hPa"}};
    case Kind::Scd4x: return {{"temperature", "°C"}, {"humidity", "%"}, {"co2", "ppm"}};
    case Kind::Bme680: return {{"temperature", "°C"}, {"humidity", "%"}, {"pressure", "Pa"},
      {"gas_resistance", "Ohm", 1}, {"gas_baseline", "Ohm", 1}, {"voc_ppb", "ppb", 1},
      {"voc_index", "a.u.", 1}, {"voc_level", "", 1}, {"voc_spikes_today", "", 1}};
    case Kind::Sds: return {{"P1", "µg/m³"}, {"P2", "µg/m³"}};
    case Kind::Noise: return {{"noiseAvg", "dBA"}, {"noiseMax", "dBA"}};
  }
  std::abort();
}

static void success(Telemetry &t, std::vector<Metric> &m, uint64_t now,
                    uint8_t mask = 1, uint8_t failed = 0, Noise *noise = nullptr) {
  CHECK(t.complete(now, mask, failed));
  for (Metric &metric : m)
    if ((mask & (1u << metric.channel)) && !(failed & (1u << metric.channel)))
      CHECK(metric.update(24, now));
  if (noise) CHECK(noise->successfulBlock(now));
}

static size_t emit(const char *scenario, Telemetry &t, const Settings &s,
                  std::vector<Metric> &m, uint64_t time, Noise *noise = nullptr, const Probe *probe = nullptr) {
  DynamicJsonDocument doc(24576);
  CHECK(formSnapshot(doc, t, s, m.data(), m.size(), "1051DB010C70", "a-b",
                     s.kind == Kind::Bme680 ? "insight" : "urban", time, noise, probe));
  CHECK(!doc.overflowed());
  std::string line;
  serializeJson(doc, line);
  CHECK(line.size() <= 8192);
  std::cout << scenario << '\t' << line << '\n';
  return line.size();
}

static void slowSensors() {
  for (Kind kind : {Kind::Bme280, Kind::Bmp280, Kind::Scd4x, Kind::Sds}) {
    Telemetry t;
    Settings s(kind, kind == Kind::Sds ? 1000 : 600000,
               kind == Kind::Sds ? 45000 : 600000, false, true);
    auto m = metrics(kind);
    const char *scenario = sensorName(kind);
    CHECK(t.due(0, false));
    emit(scenario, t, s, m, 0);
    CHECK(!t.due(30000, false));  // no periodic cached snapshots
    t.initError("init_failed");
    t.initError("init_failed");
    CHECK(t.cycle == 0 && t.channels[0].failed == 0 && t.nextFailure == 1);
    CHECK(t.due(1000, false));
    emit(scenario, t, s, m, 1000);
    CHECK(!t.due(31000, false));
    success(t, m, 600000);
    CHECK(t.channels[0].lastRecovered == 600000);
    emit(scenario, t, s, m, 600000);
    CHECK(!t.due(630000, false));
    success(t, m, 1200000);  // same values, genuinely new acquisition
    CHECK(m[0].seq == 2);
    emit(scenario, t, s, m, 1200000);
    CHECK(t.complete(1800000, 1, 1));
    t.setState(0, State::Waiting, "sds_active");
    CHECK(t.channels[0].state == State::Error);  // waiting cannot clear an error
    CHECK(m[0].seq == 2 && m[0].measured == 1200000);
    emit(scenario, t, s, m, 1800000);
    success(t, m, 2400000);
    emit(scenario, t, s, m, 2400000);
    CHECK(t.channels[0].cycles == 4 && t.channels[0].ok == 3 && t.channels[0].failed == 1);
    CHECK(!t.due(2430000, false));
  }
}

static void partialGasAndHistory() {
  Telemetry t(2);
  Settings s(Kind::Bme680, 300000, 300000, true, false);
  auto m = metrics(s.kind);
  emit("gas", t, s, m, 0);
  success(t, m, 300000, 3, 2);
  CHECK(t.failureCount == 1 && t.failure(0).channels == 2);
  CHECK(m[0].seq == 1 && m[3].seq == 0);
  emit("gas", t, s, m, 300000);
  success(t, m, 600000, 3);
  emit("gas", t, s, m, 600000);
  CHECK(t.channels[1].recovered && !t.channels[0].recovered);
  CHECK(t.complete(900000, 3, 3));
  emit("gas", t, s, m, 900000);
  CHECK(t.failureCount == 2 && t.failure(1).channels == 3);
  CHECK(t.channels[0].failed == 1 && t.channels[1].failed == 2);
  success(t, m, 1200000, 3);
  emit("gas", t, s, m, 1200000);
  for (uint64_t i = 5; i <= 24; ++i) CHECK(t.complete(i * 300000, 3, 3));
  CHECK(t.failureCount == 16 && t.failure(0).seq == 7 && t.failure(15).seq == 22);
  emit("gas", t, s, m, 7200000);

  Telemetry disabled(2);
  Settings noGas(Kind::Bme680, 300000, 300000, false, false);
  auto environment = metrics(noGas.kind);
  environment.erase(environment.begin() + 3, environment.end());
  success(disabled, environment, 300000);
  emit("gas_disabled", disabled, noGas, environment, 300000);
}

static void noiseStatistics() {
  Telemetry t;
  Settings s(Kind::Noise, 100, 0, false, true);
  Noise n(100, s.sdsBudget());
  auto m = metrics(s.kind);
  emit("noise", t, s, m, 100, &n);
  for (uint64_t time = 600; time < 300100; time += 500) success(t, m, time, 1, 0, &n);
  CHECK(n.current.blocks == 599);
  success(t, m, 300100, 1, 0, &n);
  CHECK(n.windows[0].blocks == 599 && n.current.blocks == 1);
  CHECK(n.current.seq == 2 && n.current.active == 0);
  emit("noise", t, s, m, 300100, &n);
  CHECK(n.setPause(301100, 1, true));
  t.setState(0, State::Waiting, "sds_active");
  emit("noise", t, s, m, 301100, &n);
  CHECK(n.setPause(311100, 1, false));
  CHECK(n.setPause(312100, 1, true));
  CHECK(n.advance(340100));
  CHECK(n.excluded == 20000 && n.pauseSpent == 20000);
  CHECK(n.active == 320000);  // reentry and overrun cannot renew the budget
  CHECK(!n.setPause(340100, 0, true));
  emit("noise", t, s, m, 340100, &n);
  CHECK(n.setPause(341100, 1, false));
  CHECK(t.complete(350100, 1, 1, "i2s_read_failed"));
  CHECK(n.advance(350100));
  emit("noise", t, s, m, 350100, &n);
  success(t, m, 360100, 1, 0, &n);
  CHECK(n.maxGap == 40000);  // includes the entire gap across prior snapshots
  emit("noise", t, s, m, 360100, &n);
  CHECK(n.advance(920100));
  CHECK(n.current.seq == 4 && n.windowCount == 2);
  CHECK(n.windows[0].seq == 2 && n.windows[1].seq == 3);
  CHECK(n.windows[0].blocks == 2 && n.windows[1].blocks == 0);
  CHECK(n.openGap() == 560000);
  emit("noise", t, s, m, 920100, &n);
  CHECK(!t.due(950100, true));
  success(t, m, 950101, 1, 0, &n);
  CHECK(t.due(950101, true));
  emit("noise", t, s, m, 950101, &n);

  Noise late(500, 20000);
  CHECK(late.setPause(500, 1, true));
  CHECK(late.advance(325500));
  CHECK(late.current.seq == 2 && late.windows[0].end == 320500);
  CHECK(late.current.active == 5000 && late.excluded == 20000);
  CHECK(late.setPause(325500, 1, false));
  CHECK(late.setPause(326500, 2, true));
  CHECK(late.advance(346500));
  CHECK(late.excluded == 40000);
  CHECK(late.setPause(346500, 2, false));
  CHECK(late.setPause(347500, 3, false));
  CHECK(late.hasPause && late.pauseStarted == 326500);

  Noise spanning(0, 20000);
  CHECK(spanning.advance(299000));
  CHECK(spanning.setPause(299000, 1, true));
  CHECK(spanning.advance(309000));
  spanning.formedSnapshot();
  CHECK(spanning.advance(329000));
  CHECK(spanning.windows[0].end == 320000);
  CHECK(spanning.windows[0].blocks == 0 && spanning.current.active == 9000);
  CHECK(spanning.maxGap == 309000 && spanning.maxGapEnd == 329000);

  Noise noSds(0);
  CHECK(!noSds.setPause(10, 1, true));
  CHECK(!noSds.setPause(10, 1, false));
  CHECK(noSds.advance(3000000));
  CHECK(noSds.current.seq == 11 && noSds.windows[0].seq == 9 && noSds.windows[1].seq == 10);
  CHECK(noSds.maxGap == 3000000 && noSds.maxGapEnd == 3000000);
  CHECK(!noSds.advance(2999999));
}

static void boundsAndPublication() {
  Telemetry t;
  auto m = metrics(Kind::Bme280);
  CHECK(!m[0].update(std::numeric_limits<double>::quiet_NaN(), 1));
  CHECK(m[0].seq == 0);
  CHECK(!t.complete(10, 0, 0) && !t.complete(10, 1, 2));
  CHECK(t.cycle == 0);
  t.formedSnapshot(0);
  t.initError("read_failed");
  CHECK(!t.due(999, true));
  CHECK(t.due(1000, true));
  t.formedSnapshot(1000);
  CHECK(t.complete(1001, 1, 1));
  CHECK(!t.due(2001, true));  // same error is coalesced, journal remains
  success(t, m, 2002);
  CHECK(t.due(2002, true));
  t.formedSnapshot(2002);
  success(t, m, 3000);
  CHECK(!t.due(32001, true));
  CHECK(t.due(32002, true));

  Telemetry largest(2);
  Settings s(Kind::Bme680, 300000, 300000, true, false);
  auto gas = metrics(s.kind);
  const uint64_t maxCounter = (1ULL << 53) - 1, maxClock = (1ULL << 42) - 1;
  largest.cycle = maxCounter - 16;
  largest.snapshot = maxCounter - 1;
  largest.nextFailure = maxCounter - 16;
  for (Channel &c : largest.channels) {
    c.cycles = maxCounter - 16;
    c.ok = 1;
    c.failed = maxCounter - 17;
    c.lastOk = maxClock - 1000;
    c.lastFailed = maxClock - 100;
    c.state = State::Error;
    c.reason = "read_failed";
  }
  for (Metric &metric : gas) CHECK(metric.update(24, maxClock - 1000));
  const char *reason = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  for (uint64_t i = 16; i > 0; --i) CHECK(largest.complete(maxClock - i, 3, 3, reason));
  const size_t bytes = emit("bounds", largest, s, gas, maxClock);
  std::cerr << "worst populated BME680 record: " << bytes << " bytes\n";

  DynamicJsonDocument small(256);
  CHECK(!formSnapshot(small, largest, s, gas.data(), gas.size(), "1051DB010C70", "a-b", "insight", maxClock));
  CHECK(small["_observations"]["error"] == "snapshot_overflow");
  CHECK(largest.snapshot == maxCounter + 1);  // formation consumes seq even on overflow

  Telemetry tooLong;
  Settings bme(Kind::Bme280, 300000, 300000, false, false);
  std::string text(8200, 'x');
  m[0].units = text.c_str();
  DynamicJsonDocument big(32768);
  // ArduinoJson borrows const char*, so this exercises encoded bytes, not pool exhaustion.
  CHECK(!formSnapshot(big, tooLong, bme, m.data(), m.size(), "1051DB010C70", "a-b", "urban", 0));
  CHECK(big["_observations"]["error"] == "snapshot_overflow");
}

static void ratesAndTraffic() {
  for (unsigned count : {600u, 599u, 300u, 299u}) {
    Telemetry t;
    Settings s(Kind::Noise, 100, 0, false, false);
    Noise n;
    auto m = metrics(s.kind);
    const std::string scenario = "rate_" + std::to_string(count);
    emit(scenario.c_str(), t, s, m, 0, &n);
    for (unsigned i = 0; i < count; ++i)
      success(t, m, (uint64_t(i) * 300000) / count + 1, 1, 0, &n);
    CHECK(n.advance(300000));
    CHECK(n.windows[0].blocks == count && n.current.blocks == 0);
    emit(scenario.c_str(), t, s, m, 300000, &n);
  }
  Telemetry slow;
  Settings s(Kind::Bme280, 300000, 300000, false, false);
  auto m = metrics(s.kind);
  unsigned records = 0;
  size_t bytes = 0;
  for (uint64_t time = 0; time <= 600000; time += 100) {
    if (time > 0 && time % 300000 == 0) success(slow, m, time);
    if (slow.due(time, false)) {
      bytes += emit("traffic", slow, s, m, time);
      ++records;
    }
  }
  CHECK(records == 3 && m[0].seq == 2);
  std::cerr << "600s slow-sensor replay: " << records << " records / " << bytes
            << " JSON bytes; previous 30s publication policy: 21 records\n";

  for (Kind kind : {Kind::Bme280, Kind::Sds, Kind::Noise, Kind::Scd4x, Kind::Bme680}) {
    Telemetry t(kind == Kind::Bme680 ? 2 : 1);
    Settings settings(kind, kind == Kind::Noise ? 100 : (kind == Kind::Sds ? 1000 : 300000),
                      kind == Kind::Noise ? 0 : 300000, kind == Kind::Bme680, true);
    Noise n(0, settings.sdsBudget());
    auto values = metrics(kind);
    const std::string scenario = std::string("profile_") + sensorName(kind);
    emit(scenario.c_str(), t, settings, values, 0, kind == Kind::Noise ? &n : nullptr);
  }
}


static void activityEvidence() {
  for (Kind kind : {Kind::Bme680, Kind::Noise}) {
    Telemetry t(kind == Kind::Bme680 ? 2 : 1);
    const bool noise = kind == Kind::Noise;
    Settings s(kind, noise ? 100 : 300000, noise ? 0 : 300000, !noise, false);
    auto m = metrics(kind);
    Probe p;
    Noise n;
    t.setState(noise ? 0 : 1, State::Waiting, "driver_unverified");
    const char *scenario = noise ? "probe_noise" : "probe_gas";
    emit(scenario, t, s, m, 0, noise ? &n : nullptr, &p);
    const uint64_t now = noise ? 30000 : 300000;
    if (noise) {
      p.expectedBytes = p.bytes = 8192;
      p.hasSignal = true;
      p.signalMin = -10; p.signalMax = 10;
    } else {
      success(t, m, now, 1);
      p.hasGas = true; p.gasStatus = 0; p.gasResistance = 50000;
    }
    p.record(now, true, true, "activity");
    emit(scenario, t, s, m, now, noise ? &n : nullptr, &p);
    CHECK(t.channels[noise ? 0 : 1].ok == 0);
    for (auto &metric : m) if (noise || metric.channel == 1) CHECK(metric.seq == 0);
    p.error = -1; p.bytes = 0; p.hasSignal = p.hasGas = false;
    p.record(now + 1000, false, false, "read_failed");
    emit(scenario, t, s, m, now + 1000, noise ? &n : nullptr, &p);
    CHECK(p.attempts == 2 && p.transfers == 1 && p.activity == 1);
  }
}

int main() {
  slowSensors();
  partialGasAndHistory();
  noiseStatistics();
  boundsAndPublication();
  ratesAndTraffic();
  activityEvidence();
}
