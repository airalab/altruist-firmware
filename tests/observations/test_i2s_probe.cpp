// Execute the bundled acquisition/helper/FFT path. Only the ESP I2S API is fake.
#include "sensors/drivers/i2s_noise/i2s_noise.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
uint64_t fakeNow = 0;
FakeSerial Serial;
int installError = 0, pinError = 0, startError = 0, readError = 0, samplePattern = 2;
size_t readBytes = 8192;
#define CHECK(condition) do { if (!(condition)) { \
  std::cerr << __LINE__ << ": " << #condition << '\n'; std::exit(1); \
} } while (false)
static void read(const char *expected, bool transfer, bool activity) {
  auto prior = i2sDebugProbe();
  uint8_t maximum = 0;
  float mean = 0;
  fakeNow += 1000;
  fetchSensorI2sSound(&maximum, &mean);
  const auto &p = i2sDebugProbe();
  CHECK(std::string(p.result) == expected);
  CHECK(p.attempts == prior.attempts + 1);
  CHECK(p.transfers == prior.transfers + unsigned(transfer));
  CHECK(p.activity == prior.activity + unsigned(activity));
  if (activity) CHECK(std::isfinite(mean));
}
int main() {
  installError = -1; read("init_failed", false, false); installError = 0;
  pinError = -2; read("init_failed", false, false); pinError = 0;
  startError = -3; read("read_failed", false, false); startError = 0;
  readError = -4; read("read_failed", false, false); readError = 0;
  readBytes = 0; read("short_read", false, false);
  readBytes = 100; read("short_read", false, false);
  readBytes = 8192;
  samplePattern = 0; read("no_activity", true, false);
  samplePattern = 1; read("no_activity", true, false);
  samplePattern = 3; read("no_activity", true, false);
  samplePattern = 2; read("activity", true, true);
  readError = -5; read("read_failed", false, false); readError = 0;
  read("activity", true, true);
  CHECK(i2sDebugProbe().lastActivity == fakeNow);
  std::cerr << "PASS real I2S helper: install/pin/start/read errors, zero/short blocks, "
               "zero/constant/rail signals, full activity and recovery\n";
}
