#ifndef WAVESHARE_H
#define WAVESHARE_H

#include <ArduinoJson.h>

struct main_screen_values_t {
    float pm10 = -1;
    float pm25 = -1;
    float noise_avg = -1;
    float noise_max = -1;
    float temp_outdoor = -1;
    float hum_outdoor = -1;
    float press_outdoor = -1;
    float temp_indoor = -1;
    float hum_indoor = -1;
    float co2 = -1;
};

void drawMainScreen(const String &jsonString);

#endif // WAVESHARE_H