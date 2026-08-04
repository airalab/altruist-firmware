#ifndef __CONFIG_HELPERS_H__
#define __CONFIG_HELPERS_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include "airrohr-cfg.h"
#include "defines.h"

#define JSON_BUFFER_SIZE 8192
#define FORMAT_SPIFFS_IF_FAILED true

String getConfigStringValue(const char* key);
void saveRobonomicsPrivateKey(const char* private_key);
bool writeConfig();
void readConfig(bool oldconfig = false);
void init_config();
unsigned int getConfigUintValue(const char* key);
void removeWiFiCredentials();
void removeWebUiCredentials();
bool config_set_string_by_key(const char* key, const char* value);
/** sensors.social map deep link (type, date, coords, owner, sensor). */
String buildSensorsSocialMapUrl(const char* sensor_ss58, const char* map_type = "pm10");
/**
 * Auto connectivity region when not set manually:
 * 1) valid coords_gps → RU if in rough Russia bbox, else Global
 * 2) else RU UI/build language → RU
 * Returns true if cfg::current_reg changed.
 */
bool cfgApplyAutoRegion();
/** True when coords_gps parses to a non-zero lat/lon. */
bool cfgHasValidMapCoords(double* lat_out = nullptr, double* lon_out = nullptr);

#if defined(ALTRUIST_INSIGHT)
/** Drop cached Urban SS58 / HTTP telemetry when pairing target changes. */
void clearUrbanPairingTelemetry(JsonDocument &data);
/** Standalone mode: turn off Urban night analytics and drop stale Urban PM/noise history. */
void cfgApplyStandaloneModeEnabled();
/** Paired mode: enable Urban PM/noise in sleep analytics when leaving standalone. */
void cfgOnStandaloneModeDisabled();
/** Normalize cfg UInt that may be legacy 0..23 hour or minutes 0..1439. */
unsigned cfgMinutesOfDay(unsigned raw, unsigned fallback_minutes);
/** True when local time is inside the morning analytics auto-display window (Insight). */
bool cfgInAnalyticsMorningWindow(const struct tm& timeinfo);
#endif

#endif // __CONFIG_HELPERS_H__
