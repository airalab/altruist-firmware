#include <Arduino.h>

#ifdef ESP32

#define CYCLETIME 120

bool initI2sSound(float mic_correction_db);
void fetchSensorI2sSound(uint8_t *max_noise, float *mean_noise, float mic_correction_db);

#endif // ESP32