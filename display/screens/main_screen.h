#ifdef ALTRUIST_INSIDE

#ifndef _MAIN_SCREEN_H
#define _MAIN_SCREEN_H

#include <ArduinoJson.h>
#include "../paint_driver/GUI_Paint.h"

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
    float press_indoor = -1;
    float co2 = -1;
    String ip_address = "";
};

void drawMainScreen(UBYTE *BlackImage, const String &jsonString, const String &device_ip_address);


#endif // _MAIN_SCREEN_H

#endif