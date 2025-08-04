#ifndef __WEBSERVER_UTILS_H__
#define __WEBSERVER_UTILS_H__

#include <WebServer.h>
#include <ArduinoJson.h>
#include "../config_manager/config_helpers.h"

void add_table_row_from_value(String& page_content, const String& sensor, const String& param, const String& value, const String& unit);
void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit = nullptr);
void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const __FlashStringHelper* value, const char* unit = nullptr);
void add_table_row_from_value(String& page_content, const String& param, const String& value, const char* unit = nullptr);


int32_t calcWiFiSignalQuality(int32_t rssi);
String add_sensor_type(const String& sensor_text);
String form_checkbox(const ConfigShapeId cfgid, const String& info, const bool linebreak, bool enabled = true);
String form_submit(const String& value);
String form_select_lang();
String form_select_altruist(JsonDocument& data);
String form_select_timezone();
String form_select_reg();
void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length, bool enabled);
void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length);

#endif // __WEBSERVER_UTILS_H__


