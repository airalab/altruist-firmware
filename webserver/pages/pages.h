#ifndef __PAGES_H__
#define __PAGES_H__

#include <ArduinoJson.h>
#include <WebServer.h>
#include "../../apis/rws_group.h"
#include "../../wifi_info.h"
#include "../../utils.h"

class Robonomics;

enum ScreenSaveResult : uint8_t {
	ScreenSave_None = 0,
	ScreenSave_Ok,
	ScreenSave_InvalidMode,
	ScreenSave_ConfigFailed,
};

enum ConfigHubSection : uint16_t {
	HubSec_WiFi = 1 << 0,
	HubSec_Robonomics = 1 << 1,
	HubSec_DataSharing = 1 << 2,
	HubSec_GPS = 1 << 3,
	HubSec_Auth = 1 << 4,
	HubSec_Debug = 1 << 5,
	HubSec_LEDs = 1 << 6,
	HubSec_Sleep = 1 << 7,
	HubSec_Firmware = 1 << 8,
	HubSec_WiFiConfig = 1 << 9,
	HubSec_CustomAPI = 1 << 10,
	HubSec_Influx = 1 << 11,
	HubSec_CSV = 1 << 12,
};

#define HUB_SEC_LOCAL (HubSec_WiFi | HubSec_Auth | HubSec_LEDs | HubSec_Sleep | HubSec_Firmware | HubSec_WiFiConfig)
#define HUB_SEC_SOCIAL (HubSec_Robonomics | HubSec_DataSharing | HubSec_GPS)
#define HUB_SEC_CUSTOM (HubSec_CustomAPI | HubSec_Influx | HubSec_CSV)
#define HUB_SEC_ADVANCED (HubSec_Debug)

void webserver_values(JsonDocument &data, String &page_content, WebServer &server, bool hub_embed = false);
void webserver_guest_create_body_get_part1(String& page_content, bool wificonfig_loop, device_status_t &deviceStatus);
void webserver_guest_create_body_get_part2(String& page_content, bool wificonfig_loop);
void webserver_config_send_body_post(WebServer &server);
void webserver_config_send_body_get(WebServer &server, String& page_content, bool wificonfig_loop, JsonDocument &data,
                                    const char* hub_form_action = nullptr, uint16_t hub_sections = 0,
                                    const char* sensor_ss58 = nullptr);
void webserver_hub_local(String &page_content, JsonDocument &data, device_status_t &deviceStatus, WebServer &server,
                         bool wificonfig_loop, bool readings_busy = false);
void webserver_hub_social(String &page_content, const String &robonomics_address, Robonomics *robonomics,
                          WebServer &server, JsonDocument &data, RwsGroupApplyResult group_save, bool wificonfig_loop);
void webserver_hub_custom(String &page_content, WebServer &server);
void webserver_hub_advanced(String &page_content, WebServer &server, bool wificonfig_loop);
/** Firmware Update block (check / install / switch language). */
void webserver_append_ota_section(String &page_content, device_status_t &deviceStatus, const char *form_action = "/");
void webserver_wifi(struct_wifiInfo* wifiInfo, uint8_t count_wifiInfo, String &page_content);
void webserver_debug_level(WebServer &server, String &page_content, bool hub_embed = false);
void webserver_debug_hub_section(WebServer &server, String &page_content);
void webserver_debug_log_embed(String &page_content);
void webserver_removeConfig(String &page_content, bool is_HTTP_GET, bool remove_all);
void webserver_data_json(JsonDocument &data, const String &esp_chipid, String &json_content);
void webserver_status_part1(String &page_content, device_status_t &deviceStatus, WebServer &server, bool hub_embed = false);
void webserver_status_part2(String &page_content, device_status_t &deviceStatus, WebServer &server);
void webserver_group_page(String& page_content, const String& self_ss58, Robonomics* robonomics,
                          RwsGroupApplyResult save_result, const char* form_action = "/group", bool hub_embed = false);
RwsGroupApplyResult webserver_group_post(WebServer& server, const String& self_ss58);
#ifdef ALTRUIST_INSIGHT
void webserver_screen_page(String& page_content, ScreenSaveResult save_result, const char* form_action = "/screen",
                           bool hub_embed = false);
ScreenSaveResult webserver_screen_post(WebServer& server);
#endif

#endif // __PAGES_H__