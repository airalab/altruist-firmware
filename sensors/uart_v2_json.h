#ifndef ALTRUIST_UART_V2_JSON_H
#define ALTRUIST_UART_V2_JSON_H

#if defined(ALTRUIST_BUILD_DEBUG)
#include "uart_v2.h"
#include <ArduinoJson.h>
#include <math.h>
#include <stdio.h>

namespace uart_v2 {

// The largest sensor record fits in 256 slots; reserve decimal-string storage separately.
constexpr size_t SnapshotCapacity = JSON_OBJECT_SIZE(256) + 4096;

enum class Kind { Bme280, Bmp280, Bme680, Scd4x, Sds, Noise };

inline const char *sensorName(Kind kind) {
  switch (kind) {
    case Kind::Bme280: return "BME280";
    case Kind::Bmp280: return "BMP280";
    case Kind::Bme680: return "BME680";
    case Kind::Scd4x: return "SCD4x";
    case Kind::Sds: return "SDS";
    case Kind::Noise: return "ICS43434";
  }
  return "";
}

struct Settings {
  Kind kind;
  uint64_t fetchWait, measurementInterval;
  bool gasEnabled, activeSds;

  // Supply the effective timeout and measurement interval of the sensor object,
  // not config defaults. For noise the measurement interval is unused.
  Settings(Kind sensor, uint64_t fetchWaitMs, uint64_t measurementMs,
           bool gas, bool sdsPresent)
      : kind(sensor), fetchWait(fetchWaitMs), measurementInterval(measurementMs),
        gasEnabled(gas), activeSds(sdsPresent) {}

  uint64_t sdsBudget() const {
    return kind == Kind::Sds || (kind == Kind::Noise && activeSds) ? 20000 : 0;
  }
};

struct Metric {
  const char *name;
  const char *units;
  uint8_t channel;
  double value = 0;
  uint64_t seq = 0, measured = 0;

  Metric(const char *name = nullptr, const char *units = nullptr, uint8_t channel = 0)
      : name(name), units(units), channel(channel) {}

  // Invoke only for a new valid result in a successful completed channel.
  // Equality of numbers does not imply equality of measurement identities.
  bool update(double number, uint64_t time) {
    if (!isfinite(number) || (seq && time <= measured)) return false;
    value = number;
    measured = time;
    ++seq;
    return true;
  }
};

inline void integer(JsonObject object, const char *key, uint64_t number) {
  char text[24];
  snprintf(text, sizeof(text), "%llu", static_cast<unsigned long long>(number));
  object[key] = text;
}

inline void optionalTime(JsonObject object, const char *key, uint64_t time, bool present) {
  if (present) integer(object, key, time);
  else object[key] = nullptr;
}

inline void writeSettings(JsonObject out, const Settings &s) {
  integer(out, "fetch_wait_ms", s.fetchWait);
  optionalTime(out, "measurement_interval_ms", s.measurementInterval, s.kind != Kind::Noise);
  out["publication"] = s.kind == Kind::Noise ? "coalesced" : "on_update";
  integer(out, "publish_interval_ms", s.kind == Kind::Noise ? 30000 : 0);
  integer(out, "state_min_interval_ms", 1000);
  if (s.kind == Kind::Bme680) out["gas_enabled"] = s.gasEnabled;
  else out["gas_enabled"] = nullptr;
  const bool hasSdsFields = s.kind == Kind::Sds || s.kind == Kind::Noise;
  optionalTime(out, "sds_warmup_ms", s.sdsBudget() ? 15000 : 0, hasSdsFields);
  optionalTime(out, "sds_collect_ms", s.sdsBudget() ? 5000 : 0, hasSdsFields);
  optionalTime(out, "noise_window_active_ms", Noise::WindowMs, s.kind == Kind::Noise);
}

inline void writeWindow(JsonObject out, const Window &window, bool complete) {
  integer(out, "seq", window.seq);
  integer(out, "start_ms", window.start);
  if (complete) integer(out, "end_ms", window.end);
  integer(out, "active_ms", window.active);
  integer(out, "successful_blocks", window.blocks);
}

inline void writeNoise(JsonObject out, const Noise &n) {
  integer(out, "started_ms", n.started);
  integer(out, "successful_blocks_total", n.blocks);
  integer(out, "active_ms_total", n.active);
  optionalTime(out, "last_ok_ms", n.lastOk, n.hasOk);
  JsonObject pause = out.createNestedObject("sds_pause");
  integer(pause, "cycle_seq", n.pauseCycle);
  pause["state"] = n.paused ? "active" : "idle";
  optionalTime(pause, "started_ms", n.pauseStarted, n.hasPause);
  integer(pause, "excluded_ms_total", n.excluded);
  JsonObject interval = out.createNestedObject("interval");
  integer(interval, "start_ms", n.intervalStart);
  integer(interval, "end_ms", n.now);
  integer(interval, "max_gap_active_ms", n.maxGap);
  integer(interval, "max_gap_end_ms", n.maxGapEnd);
  integer(interval, "open_gap_active_ms", n.openGap());
  JsonArray windows = out.createNestedArray("windows");
  for (size_t i = 0; i < n.windowCount; ++i) writeWindow(windows.createNestedObject(), n.windows[i], true);
  writeWindow(out.createNestedObject("window_current"), n.current, false);
}

inline void writeProbe(JsonObject out, const Probe &p) {
  integer(out, "attempts_total", p.attempts);
  integer(out, "transfers_total", p.transfers);
  integer(out, "activity_total", p.activity);
  optionalTime(out, "last_attempt_ms", p.lastAttempt, p.attempts > 0);
  optionalTime(out, "last_transfer_ms", p.lastTransfer, p.transfers > 0);
  optionalTime(out, "last_activity_ms", p.lastActivity, p.activity > 0);
  out["result"] = p.result;
  out["error_code"] = p.error;
  integer(out, "bytes_read", p.bytes);
  integer(out, "expected_bytes", p.expectedBytes);
  if (p.hasSignal) {
    out["signal_min"] = p.signalMin;
    out["signal_max"] = p.signalMax;
  } else { out["signal_min"] = nullptr; out["signal_max"] = nullptr; }
  if (p.hasGas) {
    out["gas_status"] = p.gasStatus;
    integer(out, "gas_resistance", p.gasResistance);
  } else { out["gas_status"] = nullptr; out["gas_resistance"] = nullptr; }
}

// The caller owns the UART-only document. Form it once, serialize once and
// append LF in a single Serial.write; never add this metadata to sensors_data.
// Return the measured wire length, or zero so the caller can send a fixed marker.
inline size_t formSnapshot(JsonDocument &doc, Telemetry &t, const Settings &s,
                         Metric *metrics, size_t metricCount,
                         const char *device, const char *boot, const char *model,
                         uint64_t now, Noise *noise = nullptr, const Probe *probe = nullptr) {
  doc.clear();
  t.formedSnapshot(now);
  JsonObject values = doc.createNestedObject(sensorName(s.kind));
  JsonObject meta = doc.createNestedObject("_observations");
  meta["version"] = 2;
  meta["device_id"] = device;
  meta["boot_id"] = boot;
  meta["model"] = model;
  meta["sensor"] = sensorName(s.kind);
  integer(meta, "snapshot_seq", t.snapshot);
  integer(meta, "cycle_seq", t.cycle);
  integer(meta, "time_ms", now);
  const Channel *status = &t.channels[0];
  const uint8_t requiredChannels = s.kind == Kind::Bme680 && !s.gasEnabled ? 1 : t.channelCount;
  for (uint8_t i = 0; i < requiredChannels; ++i) {
    if (t.channels[i].state == State::Error) { status = &t.channels[i]; break; }
    if (t.channels[i].state != State::Ok) status = &t.channels[i];
  }
  meta["state"] = stateName(status->state);
  meta["reason"] = status->reason;
  if (s.kind == Kind::Noise && probe && noise && noise->paused) meta["reason"] = "sds_active";
  if (status->state == State::Never && t.cycle > 0) {
    meta["state"] = "waiting";
    meta["reason"] = "channel_pending";
  }
  writeSettings(meta.createNestedObject("settings"), s);
  if (probe) {
    JsonObject probes = meta.createNestedObject("probes");
    writeProbe(probes.createNestedObject(s.kind == Kind::Bme680 ? "gas" : "main"), *probe);
  }
  JsonObject channels = meta.createNestedObject("channels");
  for (uint8_t i = 0; i < t.channelCount; ++i) {
    const Channel &c = t.channels[i];
    const char *name = s.kind == Kind::Bme680 ? (i == 0 ? "environment" : "gas") : "main";
    JsonObject channel = channels.createNestedObject(name);
    integer(channel, "cycles_total", c.ok + c.failed);
    integer(channel, "ok_total", c.ok);
    integer(channel, "failed_total", c.failed);
    optionalTime(channel, "last_ok_ms", c.lastOk, c.ok > 0);
    optionalTime(channel, "last_failed_ms", c.lastFailed, c.failed > 0);
    optionalTime(channel, "last_recovered_ms", c.lastRecovered, c.recovered);
    channel["state"] = stateName(c.state);
    channel["reason"] = c.reason;
  }
  JsonObject history = meta.createNestedObject("failures");
  integer(history, "next_seq", t.nextFailure);
  JsonArray recent = history.createNestedArray("recent");
  for (size_t i = 0; i < t.failureCount; ++i) {
    const Failure &f = t.failure(i);
    JsonObject entry = recent.createNestedObject();
    integer(entry, "seq", t.nextFailure - t.failureCount + i);
    integer(entry, "cycle_seq", f.cycle);
    integer(entry, "at_ms", f.at);
    entry["reason"] = f.reason;
    JsonArray names = entry.createNestedArray("channels");
    for (uint8_t j = 0; j < t.channelCount; ++j)
      if (f.channels & (1u << j)) names.add(s.kind == Kind::Bme680 ? (j == 0 ? "environment" : "gas") : "main");
  }
  bool valid = metricCount <= 16;
  for (size_t i = 0; i < metricCount && i < 16; ++i) {
    const Metric &m = metrics[i];
    if (m.channel >= t.channelCount) { valid = false; continue; }
    JsonObject metric = values.createNestedObject(m.name);
    if (m.seq) metric["value"] = m.value;
    else metric["value"] = nullptr;
    metric["units"] = m.units;
    JsonObject observation = metric.createNestedObject("observation");
    integer(observation, "seq", m.seq);
    optionalTime(observation, "measured_ms", m.measured, m.seq > 0);
    const Channel &c = t.channels[m.channel];
    observation["state"] = stateName(c.state == State::Ok && m.seq == 0 ? State::Warming : c.state);
  }
  if (s.kind == Kind::Noise) {
    if (!noise || !noise->advance(now)) valid = false;
    else {
      writeNoise(meta.createNestedObject("noise"), *noise);
      noise->formedSnapshot();
    }
  }
  const size_t length = valid && !doc.overflowed() ? measureJson(doc) : 0;
  return length <= 8192 ? length : 0;
}

}  // namespace uart_v2
#endif  // ALTRUIST_BUILD_DEBUG
#endif
