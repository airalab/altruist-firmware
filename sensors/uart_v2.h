#ifndef ALTRUIST_UART_V2_H
#define ALTRUIST_UART_V2_H

#if defined(ALTRUIST_BUILD_DEBUG)

// Debug telemetry state. The caller supplies monotonic esp_timer milliseconds,
// effective settings and completed acquisition results, never cached-field writes.
// No Arduino/RTOS dependencies: the same state machine runs in host regressions.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace uart_v2 {

// Acquisition evidence only. These counters never advance measurement identities.
struct Probe {
  uint64_t attempts = 0, transfers = 0, activity = 0;
  uint64_t lastAttempt = 0, lastTransfer = 0, lastActivity = 0;
  const char *result = "never";
  int32_t error = 0, signalMin = 0, signalMax = 0;
  uint32_t bytes = 0, expectedBytes = 0, gasResistance = 0;
  uint8_t gasStatus = 0;
  bool hasSignal = false, hasGas = false;

  void record(uint64_t time, bool transferred, bool active, const char *code) {
    ++attempts;
    lastAttempt = time;
    result = code;
    if (transferred) { ++transfers; lastTransfer = time; }
    if (transferred && active) { ++activity; lastActivity = time; }
  }
};

enum class State { Never, Warming, Waiting, Ok, Error };
inline const char *stateName(State state) {
  switch (state) {
    case State::Never: return "never";
    case State::Warming: return "warming";
    case State::Waiting: return "waiting";
    case State::Ok: return "ok";
    case State::Error: return "error";
  }
  return "error";
}

struct Channel {
  uint64_t cycles = 0, ok = 0, failed = 0;
  uint64_t lastOk = 0, lastFailed = 0, lastRecovered = 0;
  bool recovered = false;
  State state = State::Never;
  const char *reason = "none";
};

struct Failure {
  uint64_t seq = 0, cycle = 0, at = 0;
  uint8_t channels = 0;
  const char *reason = "none";
};

struct Window {
  uint64_t seq = 1, start = 0, end = 0, active = 0, blocks = 0;
};

struct Noise {
  static constexpr uint64_t WindowMs = 300000;
  uint64_t started = 0, now = 0, active = 0, blocks = 0;
  uint64_t lastOk = 0, lastOkActive = 0;
  bool hasOk = false;
  uint64_t pauseCycle = 0, pauseStarted = 0, excluded = 0, pauseSpent = 0;
  uint64_t pauseBudget = 0;
  bool paused = false, hasPause = false, cycleHasPause = false;
  uint64_t intervalStart = 0, maxGap = 0, maxGapEnd = 0;
  Window current, windows[2];
  size_t windowCount = 0;

  explicit Noise(uint64_t start = 0, uint64_t sdsBudget = 0)
      : started(start), now(start), pauseBudget(sdsBudget),
        intervalStart(start), maxGapEnd(start) {
    current.start = start;
  }

  uint64_t openGap() const { return active - lastOkActive; }

  void rememberWindow(const Window &window) {
    if (windowCount == 2) windows[0] = windows[1];
    else ++windowCount;
    windows[windowCount - 1] = window;
  }

  // Call at acquisition completion and at the actual SDS state boundaries.
  // This also closes zero-success windows during periods without UART output.
  bool advance(uint64_t time) {
    if (time < now) return false;
    const uint64_t elapsed = time - now;
    const uint64_t remaining = pauseBudget - pauseSpent;
    const uint64_t skip = paused ? (elapsed < remaining ? elapsed : remaining) : 0;
    excluded += skip;
    pauseSpent += skip;
    uint64_t cursor = now + skip;
    uint64_t duration = elapsed - skip;
    active += duration;
    if (duration >= WindowMs - current.active) {
      const uint64_t step = WindowMs - current.active;
      cursor += step;
      duration -= step;
      current.active = WindowMs;
      current.end = cursor;
      rememberWindow(current);
      const uint64_t next = current.seq + 1;
      current = Window{};
      current.seq = next;
      current.start = cursor;
      // No blocks occurred between calls. Retain only the two latest empty
      // windows without looping over a long worker stall.
      const uint64_t empty = duration / WindowMs;
      for (uint64_t i = empty > 2 ? empty - 2 : 0; i < empty; ++i) {
        Window window;
        window.seq = next + i;
        window.start = cursor + i * WindowMs;
        window.end = window.start + WindowMs;
        window.active = WindowMs;
        rememberWindow(window);
      }
      current.seq += empty;
      current.start += empty * WindowMs;
      duration %= WindowMs;
    }
    current.active += duration;
    now = time;
    const uint64_t gap = openGap();
    if (gap >= maxGap) {
      maxGap = gap;
      maxGapEnd = time;
    }
    return true;
  }

  bool setPause(uint64_t time, uint64_t cycle, bool running) {
    if (time < now || cycle < pauseCycle ||
        (pauseBudget == 0 && cycle != 0) ||
        (running && (pauseBudget == 0 || cycle == 0))) return false;
    advance(time);
    if (cycle > pauseCycle) {
      pauseCycle = cycle;
      pauseSpent = 0;
      cycleHasPause = false;
    }
    if (running && !cycleHasPause) {
      pauseStarted = time;
      hasPause = true;
      cycleHasPause = true;
    }
    paused = running;
    return true;
  }

  bool successfulBlock(uint64_t time) {
    if (!advance(time)) return false;
    ++blocks;
    ++current.blocks;  // A block completing on a boundary belongs to the new window.
    lastOk = time;
    lastOkActive = active;
    hasOk = true;
    return true;
  }

  void formedSnapshot() {
    intervalStart = now;
    maxGap = openGap();
    maxGapEnd = now;
  }
};

struct Telemetry {
  Channel channels[2];
  Failure failures[16];
  size_t failureCount = 0, failureHead = 0;
  uint64_t cycle = 0, nextFailure = 1, snapshot = 0, lastPrint = 0;
  uint64_t publishedOk = 0;
  bool statePending = true, updatePending = false;
  uint8_t channelCount;

  explicit Telemetry(uint8_t count = 1) : channelCount(count) {}

  // Reasons are stable codes with static lifetime, not temporary String buffers.
  void setState(uint8_t index, State state, const char *reason) {
    Channel &c = channels[index];
    if (c.state == State::Error && state != State::Error) return;
    if (c.state != state || strcmp(c.reason, reason) != 0) statePending = true;
    c.state = state;
    c.reason = reason;
  }

  void initError(const char *reason, uint8_t mask = 1) {
    for (uint8_t i = 0; i < channelCount; ++i)
      if (mask & (1u << i)) setState(i, State::Error, reason);
  }

  // Exactly one call per completed physical cycle. A shared BME680 transport
  // failure supplies mask=3/failedMask=3, producing one journal record.
  bool complete(uint64_t time, uint8_t mask, uint8_t failedMask,
                const char *reason = "read_failed") {
    const uint8_t valid = (1u << channelCount) - 1;
    if (!mask || (mask & ~valid) || (failedMask & ~mask)) return false;
    ++cycle;
    updatePending = true;
    for (uint8_t i = 0; i < channelCount; ++i) {
      if (!(mask & (1u << i))) continue;
      Channel &c = channels[i];
      ++c.cycles;
      if (failedMask & (1u << i)) {
        ++c.failed;
        c.lastFailed = time;
        setState(i, State::Error, reason);
      } else {
        if (c.state == State::Error) {
          c.lastRecovered = time;
          c.recovered = true;
        }
        if (c.state != State::Ok) statePending = true;
        ++c.ok;
        c.lastOk = time;
        c.state = State::Ok;
        c.reason = "none";
      }
    }
    if (failedMask) {
      Failure &f = failures[failureHead];
      f.seq = nextFailure++;
      f.cycle = cycle;
      f.at = time;
      f.channels = failedMask;
      f.reason = reason;
      failureHead = (failureHead + 1) % 16;
      if (failureCount < 16) ++failureCount;
    }
    return true;
  }

  const Failure &failure(size_t index) const {
    return failures[(failureHead + 16 - failureCount + index) % 16];
  }

  bool due(uint64_t now, bool noise) const {
    if (snapshot == 0) return true;
    if (now < lastPrint || now - lastPrint < 1000) return false;
    if (statePending) return true;
    return noise ? channels[0].ok > publishedOk && now - lastPrint >= 30000 : updatePending;
  }

  // Called when a complete document is formed, even if the write is lost.
  void formedSnapshot(uint64_t now) {
    ++snapshot;
    lastPrint = now;
    publishedOk = channels[0].ok;
    statePending = false;
    updatePending = false;
  }
};

}  // namespace uart_v2
#endif  // ALTRUIST_BUILD_DEBUG
#endif
