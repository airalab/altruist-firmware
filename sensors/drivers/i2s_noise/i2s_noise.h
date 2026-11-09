#include <Arduino.h>

#ifdef ESP32

#define CYCLETIME 120

bool initI2sSound();
void fetchSensorI2sSound(uint8_t *max_noise, float *mean_noise);

#endif // ESP32