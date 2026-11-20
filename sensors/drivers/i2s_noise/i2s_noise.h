#include <Arduino.h>

#ifdef ESP32

#define CYCLETIME 120

bool initI2sSound();
/** true only after a complete I2S block. Outputs unchanged on failure. */
bool fetchSensorI2sSound(uint8_t *max_noise, float *mean_noise);
#if defined(ALTRUIST_BUILD_DEBUG)
#include "../../uart_v2.h"
uart_v2::Probe &i2sDebugProbe();
#endif

#endif // ESP32
