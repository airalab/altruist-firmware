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

#if defined(ALTRUIST_INSIGHT)
/** Drop cached Urban SS58 / HTTP telemetry when pairing target changes. */
void clearUrbanPairingTelemetry(JsonDocument &data);
/** Standalone mode: turn off Urban night analytics and drop stale Urban PM/noise history. */
void cfgApplyStandaloneModeEnabled();
/** Paired mode: enable Urban PM/noise in sleep analytics when leaving standalone. */
void cfgOnStandaloneModeDisabled();
#endif

#endif // __CONFIG_HELPERS_H__
