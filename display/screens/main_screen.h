#ifdef ALTRUIST_INSIGHT

#ifndef _MAIN_SCREEN_H
#define _MAIN_SCREEN_H

#include <ArduinoJson.h>
#include "../paint_driver/GUI_Paint.h"

struct main_screen_values_t {
    float pm10 = -1;
    float pm25 = -1;
    float noise_avg = -1;
    float noise_max = -1;
    // Use a sentinel outside valid temperature range so -1°C can be displayed.
    float temp_outdoor = -1000;
    float hum_outdoor = -1;
    float press_outdoor = -1;
    // Use a sentinel outside valid temperature range so -1°C can be displayed.
    float temp_indoor = -1000;
    float hum_indoor = -1;
    float press_indoor = -1;
    float co2 = -1;
    String ip_address = "";
    String urban_robonomics_address = "";

    // Urban connectivity freshness (TTL)
    // 0 = unknown/online, 1 = stale, 2 = offline
    uint8_t urban_ttl_state = 0;
    uint16_t urban_age_min = 0;

    /** Insight STA: connected and usable IPv4 (see wifiStaLinkReady). */
    bool wifi_sta_link_ok = false;
};

void extractMainScreenValues(const JsonDocument &data, main_screen_values_t &values);
void drawMainScreen(UBYTE *BlackImage, const main_screen_values_t &values, const String &device_ip_address, 
                    const String &insight_robonomics_address = "", const String &urban_robonomics_address = "");


#endif // _MAIN_SCREEN_H

#endif
