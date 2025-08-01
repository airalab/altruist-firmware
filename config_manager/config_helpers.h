#ifndef __CONFIG_HELPERS_H__
#define __CONFIG_HELPERS_H__

#include <Arduino.h>
#include "airrohr-cfg.h"
#include "defines.h"

#define JSON_BUFFER_SIZE 2800
#define FORMAT_SPIFFS_IF_FAILED true

String getConfigStringValue(const char* key);
void saveRobonomicsPrivateKey(const char* private_key);
bool writeConfig();
void readConfig(bool oldconfig = false);
void init_config();
unsigned int getConfigUintValue(const char* key);
void removeWiFiCredentials();
bool config_set_string_by_key(const char* key, const char* value);

#endif // __CONFIG_HELPERS_H__
