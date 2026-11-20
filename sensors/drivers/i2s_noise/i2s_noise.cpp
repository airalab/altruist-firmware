/*--------------------------------------------------------------------
  LoRa Soundkit
  Measures environtmentalsound and send data to LoRa network

  Author Marcel Meek
  Date 12/7/2020
  changed 18-3-2024
  - Sources adapted for LilyGO TTGO T3 LoRa32 board (10dB better radio performance)
  - Sources adapted for VC PlatformIO (arduino IDE support not tested)
  - During a TTN connect phase, the I2S MEMS driver is stopped, because it gives Rx radio interference
  - The update of the OLED display is moved from the sound thread to the main thread.
  changed 15-8-2021
  - MCCI Catena LoRa stack
  - Worker loop changed (hang situation solved with TTN V3)
  - OLED display added
  - DEVEUI obtained from BoardID, (same SW for multiple sensors)
  - use the TWO processor cores of ESP (one core for audio and one core for LoRa)
  - DC offset MEMS compensated by moving average window
  - payload compressed from 27 to 19 bytes
  Changed 1/11/2023
  - Joining TTN problems solved, join was disturbed by I2S driver, it is stopped during join
  - Working loop improved, sleep is postponed after send, and a shorter sleep during joining phase 
  - TTN SF9 is the default and ADR is enabled
  --------------------------------------------------------------------*/

#ifdef ESP32

#include "i2s_noise.h"
#include "soundsensor.h"
#include "measurement.h"


// Weighting lists
static float aweighting[] = A_WEIGHTING;

// measurement buffers, filled by core 0, read by core 1
static Measurement aMeasurement( aweighting);

// task semaphores
static bool audioRequest = false;
static bool audioReady = false;
static bool sound = false;

static SoundSensor soundSensor;
#if defined(ALTRUIST_BUILD_DEBUG)
#include <esp_timer.h>
uart_v2::Probe &i2sDebugProbe() {
  static uart_v2::Probe probe;
  return probe;
}
#endif

bool initI2sSound() {
  return soundSensor.begin();
}

bool fetchSensorI2sSound(uint8_t *max_noise, float *mean_noise) {
  if (!initI2sSound()) {
#if defined(ALTRUIST_BUILD_DEBUG)
    auto &probe = i2sDebugProbe();
    probe.bytes = 0;
    probe.hasSignal = false;
    probe.record(static_cast<uint64_t>(esp_timer_get_time()) / 1000, false, false, "init_failed");
#endif
    return false;
  }
  if (!soundSensor.running()) {
    if (!soundSensor.start()) {
      soundSensor.disable();
      return false;
    }
  }
  float *energy = soundSensor.readSamples();
  if (!energy) {
    soundSensor.stop();
    soundSensor.disable();
    return false;
  }

  aMeasurement.update(energy);
  aMeasurement.calculate();

  if (mean_noise) {
    *mean_noise = aMeasurement.avg;
  }
  if (max_noise) {
    *max_noise = static_cast<uint8_t>(aMeasurement.max);
  }
  soundSensor.stop();
  soundSensor.disable();
  return true;
}

#endif // ESP32