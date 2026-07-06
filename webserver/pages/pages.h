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

void webserver_values(JsonDocument &data, String &page_content);
void webserver_guest_create_body_get_part1(String& page_content, bool wificonfig_loop, device_status_t &deviceStatus);
void webserver_guest_create_body_get_part2(String& page_content, bool wificonfig_loop);
void webserver_config_send_body_post(WebServer &server);
void webserver_config_send_body_get(WebServer &server, String& page_content, bool wificonfig_loop, JsonDocument &data);
void webserver_root(String &page_content, const String &robonomics_address);
void webserver_wifi(struct_wifiInfo* wifiInfo, uint8_t count_wifiInfo, String &page_content);
void webserver_debug_level(WebServer &server, String &page_content);
void webserver_removeConfig(String &page_content, bool is_HTTP_GET, bool remove_all);
void webserver_data_json(JsonDocument &data, const String &esp_chipid, String &json_content);
void webserver_status_part1(String &page_content, device_status_t &deviceStatus);
void webserver_status_part2(String &page_content, device_status_t &deviceStatus);
void webserver_group_page(String& page_content, const String& self_ss58, Robonomics* robonomics,
                          RwsGroupApplyResult save_result);
RwsGroupApplyResult webserver_group_post(WebServer& server, const String& self_ss58);
#ifdef ALTRUIST_INSIGHT
void webserver_screen_page(String& page_content, ScreenSaveResult save_result);
ScreenSaveResult webserver_screen_post(WebServer& server);
#endif

#endif // __PAGES_H__