#ifdef ALTRUIST_INSIDE

#ifndef _ANALYTICS_SCREEN_H
#define _ANALYTICS_SCREEN_H

#include <ArduinoJson.h>
#include <WString.h>
#include "../driver/EPD.h"

enum class analytics_view_t : uint8_t {
    OVERVIEW_24H = 0
};

struct analytics_metric_t {
    float current = 0.0f;
    float min24h = 0.0f;
    float max24h = 0.0f;
    float avg24h = 0.0f;
    bool has_current = false;
    bool has_24h = false;
};

struct analytics_screen_values_t {
    // Climate
    analytics_metric_t temp_indoor;
    analytics_metric_t hum_indoor;
    analytics_metric_t dew_indoor;
    analytics_metric_t temp_urban;
    analytics_metric_t hum_urban;
    // Air
    analytics_metric_t co2;
    analytics_metric_t pm25;
    analytics_metric_t pm10_insight;
    analytics_metric_t pm25_insight;
    // Noise
    analytics_metric_t noise_avg;
};

analytics_view_t analyticsGetView();
bool analyticsNextViewAtEdge();
bool analyticsPrevViewAtEdge();
const char* analyticsViewLabel();
bool analyticsHistoryPersistenceEnabled();
bool analyticsHistoryIsLoaded();
bool analyticsHistoryHasData();

void extractAnalyticsScreenValues(const DynamicJsonDocument &doc, analytics_screen_values_t &values);
void analyticsIngestHourSample(const analytics_screen_values_t &values);
void analyticsDevLogStatus15m();
void populateAnalyticsPeriodStats(analytics_screen_values_t &values);
void showAnalyticsPage(UBYTE *BlackImage, const analytics_screen_values_t &values, const String &sensor_map_address);

#endif // _ANALYTICS_SCREEN_H

#endif
