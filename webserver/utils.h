#ifndef __WEBSERVER_UTILS_H__
#define __WEBSERVER_UTILS_H__

#include <WebServer.h>
#include <ArduinoJson.h>
#include "../config_manager/config_helpers.h"

/** Flush a built HTML chunk over chunked HTTP and yield so sensors/WiFi keep running. */
void web_page_flush_chunk(String& page_content, WebServer* server);
/** Stream PROGMEM bytes into the current chunked HTTP response. */
void web_send_content_progmem(WebServer* server, const char* data, size_t len);
/** Send the final empty chunk to close a chunked HTTP response. */
void web_page_finish_chunked(WebServer* server);

void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit = nullptr);
void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const __FlashStringHelper* value, const char* unit = nullptr);
void add_table_row_from_value(String& page_content, const String& param, const String& value, const char* unit = nullptr);

void add_data_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit = nullptr);
void add_data_row_from_value(String& page_content, const __FlashStringHelper* param, const __FlashStringHelper* value, const char* unit = nullptr);
void add_data_row_from_value(String& page_content, const String& param, const String& value, const char* unit = nullptr);

void add_data_section_start(String& page_content, const __FlashStringHelper* label, const char* block_modifier = nullptr);
void add_data_section_start(String& page_content, const String& label, const char* block_modifier = nullptr);
void add_data_section_end(String& page_content);
void add_data_api_status_row(String& page_content, const String& api_name, const String& status,
	const String& sends, const String& last_send);

void add_reading_metrics_grid_start(String& page_content);
void add_reading_metrics_grid_end(String& page_content);
void add_reading_metric_card(String& page_content, const __FlashStringHelper* label, const String& value, const char* unit = nullptr);
void add_reading_metric_card(String& page_content, const String& label, const String& value, const char* unit = nullptr);
void add_data_block_intro(String& page_content, const __FlashStringHelper* intro);

int32_t calcWiFiSignalQuality(int32_t rssi);
/** Append one Wi‑Fi row to page_content (no returned String — safe on small HTTP-handler stack). */
void append_wlan_ssid_table_row(String& page_content, const char* ssid, uint8_t encryptionType, int32_t rssi);
String add_sensor_type(const String& sensor_text);
String form_checkbox(const ConfigShapeId cfgid, const String& info, const bool linebreak, bool enabled = true);
String form_submit(const String& value);
String form_select_lang();
String form_select_altruist(JsonDocument& data);
String form_select_timezone();
String form_select_reg();
void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length, bool enabled);
void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length);

String buildLocalAccessLabel();
/** Real device hostname for mDNS/URLs (may include model + short id). */
String buildDeviceAccessHost();
struct device_status_t;
/** Fill {device_chip}/{send_chip}/{tags}/{host}/{ip}/{device}/{addr} in WEB_PAGE_APP_TOPBAR_BODY. */
void fill_app_topbar_placeholders(String& topbar, const device_status_t& deviceStatus,
                                  const String& chipid, const String& robonomics_addr);
/**
 * Guest setup success: show IP + sensor address inline, plus a download button
 * that saves JSON without navigating away from the page.
 */
void append_guest_device_access(String& page_content, const String& ip, const String& sensor_ss58);
/** Countdown hint + Finish setup button (POST /finish_setup) for guest success pages. */
void append_guest_success_restart_ui(String& page_content);
/** Restore-from-backup form (Advanced + guest Wi‑Fi setup). guest_mode uses a visible file input (iOS-safe). */
void append_device_backup_restore_form(String& page_content, const __FlashStringHelper* hint = nullptr, bool guest_mode = false);
void append_app_sidebar(String& page_content);
void append_app_page_body_start(String& page_content, const __FlashStringHelper* lead = nullptr);
void append_app_page_body_end(String& page_content);

void append_hub_page_start(String& page_content);
void append_hub_page_end(String& page_content);
void append_hub_group_start(String& page_content, const __FlashStringHelper* title,
                            const __FlashStringHelper* intro = nullptr, const char* modifier = nullptr);
void append_hub_group_end(String& page_content);
void append_hub_section_start(String& page_content, const __FlashStringHelper* title, const char* section_id = nullptr);
void append_hub_section_end(String& page_content);
void append_hub_config_form_start(String& page_content, const char* form_action);
void append_hub_config_form_end(String& page_content, bool load_wifi_list = false);

#endif // __WEBSERVER_UTILS_H__


