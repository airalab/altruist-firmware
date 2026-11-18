#include <Arduino.h>

#ifdef ESP32

#define CYCLETIME 120

bool initI2sSound();
void fetchSensorI2sSound(uint8_t *max_noise, float *mean_noise);
#if defined(ALTRUIST_BUILD_DEBUG)
#include "../../uart_v2.h"
uart_v2::Probe &i2sDebugProbe();
#endif

#endif // ESP32