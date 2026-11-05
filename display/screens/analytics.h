#ifdef ALTRUIST_INSIGHT

#ifndef _ANALYTICS_SCREEN_H
#define _ANALYTICS_SCREEN_H

#include <ArduinoJson.h>
#include <WString.h>
#include "../driver/EPD.h"
#include "../../defines.h"

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
    analytics_metric_t press_indoor;
    analytics_metric_t press_urban;
    // Air
    analytics_metric_t co2;
    analytics_metric_t pm25;
    analytics_metric_t pm10;
    analytics_metric_t pm10_insight;
    analytics_metric_t pm25_insight;
    // Noise
    analytics_metric_t noise_avg;
    analytics_metric_t noise_max;
    // VOC organic events today (−1 = sensor absent)
    int16_t voc_spikes_today = -1;
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

enum class analytics_hour_series_t : uint8_t {
    TempIndoor = 0,
    HumIndoor,
    TempUrban,
    HumUrban,
    Pm25,
    Co2,
    Noise,
    PressIndoor,
    PressUrban,
    Pm10,
    NoiseAvg
};

struct analytics_last_hours_t {
    uint8_t n = DESK_GRAPH_HOURS;
    uint8_t found = 0;
    bool has[DESK_GRAPH_HOURS] = {};
    float avg[DESK_GRAPH_HOURS] = {};
    float maxv[DESK_GRAPH_HOURS] = {};
    float minv[DESK_GRAPH_HOURS] = {};
    uint8_t hour[DESK_GRAPH_HOURS] = {};
};

void analyticsReadLastHours(analytics_hour_series_t series, analytics_last_hours_t &out);
int analyticsPrepareMapQr(const char *qr_url, int max_w, int max_h);
void analyticsDrawPreparedMapQr(int x, int y);
void showAnalyticsPage(UBYTE *BlackImage, const analytics_screen_values_t &values,
                       const String &sensor_ss58 = "");
/** Clear PM2.5/noise night buckets that came from Urban (when switching to standalone). */
void analyticsClearUrbanNightHistory();

#endif // _ANALYTICS_SCREEN_H

#endif