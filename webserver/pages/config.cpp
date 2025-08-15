#include "pages.h"
#include "../../config_manager/config_helpers.h"
#include "../../utils.h"
#include "../html-content.h"
#include "../utils.h"

void webserver_config_send_body_post(WebServer &server) {
	String masked_pwd;

	for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());
		if (!server.hasArg(s_param)) {
			continue;
		}
		const String server_arg(server.arg(s_param));

		switch (c.cfg_type) {
		case Config_Type_UInt:
			*(c.cfg_val.as_uint) = server_arg.toInt();
			break;
		case Config_Type_Time:
			*(c.cfg_val.as_uint) = server_arg.toInt() * 1000;
			break;
		case Config_Type_Bool:
			*(c.cfg_val.as_bool) = (server_arg == "1");
			break;
		case Config_Type_String:
			strncpy(c.cfg_val.as_str, server_arg.c_str(), c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
			break;
		case Config_Type_Password:
			if (server_arg.length()) {
				server_arg.toCharArray(c.cfg_val.as_str, LEN_CFG_PASSWORD);
			}
			break;
		}
	}
}

/*****************************************************************
 * Webserver config: show config page                            *
 *****************************************************************/

void webserver_config_send_body_get(WebServer &server, String& page_content, bool wificonfig_loop, JsonDocument &data) {
	auto add_form_checkbox = [&page_content](const ConfigShapeId cfgid, const String& info, bool enabled) {
		page_content += form_checkbox(cfgid, info, true, enabled);
	};


	// изменения в верстке
	debug_outln_info(F("begin webserver_config_body_get ..."));
	page_content += F("<form method='POST' action='/config' style='width:100%;'>\n"
  "<div class='tabs'>"
	"<div class='tab' data-id='1' style='background: rgb(244, 244, 244)'>" INTL_COMMON_SETTINGS "</div>"
	"<div class='tab' data-id='2' style='background: rgb(244, 244, 244)'>");
	page_content += FPSTR(INTL_MORE_SETTINGS);
	page_content += F("</div>"
		"<div class='tab' data-id='3' style='background: rgb(244, 244, 244)'>" INTL_APIS_SETTINGS "</div></div>"
		"<div class='panel' id='panel1'>");

	// if (wificonfig_loop) {  // scan for wlan ssids
	// 	page_content += F("<div id='wifilist'>" INTL_WIFI_NETWORKS "</div><br/>");
	// }

	// WiFi Settings (tab 1)

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_WIFI "</h3>");
	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID-1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += form_checkbox(Config_wlannopwd, FPSTR(INTL_NO_WLAN_PWD), false);
	add_form_input(page_content, Config_local_hostname, FPSTR(INTL_LOCAL_HOSTNAME), LEN_LOCAL_HOSTNAME-1);
	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	// Robonomics Settings (tab 1)

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_ROBONOMICS "</h3>");
	// page_content += form_checkbox(Config_send2robonomics, FPSTR(WEB_ROBONOMICS), false);
	add_form_input(page_content, Config_rws_owner, FPSTR(INTL_RWS_OWNER), LEN_RWS_OWNER-1);
	add_form_input(page_content, Config_datalog_sending_intervall_ms, FPSTR(INTL_DATALOG_SENDING_INTERVAL), 5);
	add_form_input(page_content, Config_robonomics_public_node, FPSTR(INTL_ROBONOMICS_PUBLIC_NODE), LEN_ROBONOMICS_PUBLIC_NODE-1);
	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	// GPS Settings (tab 1)

	page_content += F("<div class='panel-container panel-container--with-map'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_GPS "</h3>");
	add_form_input(page_content, Config_coords_gps, FPSTR(INTL_COORDS), LEN_GPS_COORDS-1);
	add_form_input(page_content, Config_temp_correction, FPSTR(INTL_TEMP_CORRECTION), LEN_TEMP_CORRECTION-1);
#ifdef ALTRUIST_URBAN
	add_form_input(page_content, Config_sds_meas_interval_ms, FPSTR(INTL_SDS_MEAS_INTERVAL), 5);
#endif
#ifdef ALTRUIST_INSIDE
	page_content += form_select_altruist(data);
#endif
	page_content += F("<div class='map-container'><div id='map'></div>");
	page_content += F("</div><span class='map-text'> <em>The marker on the map shows approximate location to make sure you have the right hemisphere</em></span>");
	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	// Authentication  (tab 2)
	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(2));

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_AUTH "</h3>");

	add_form_checkbox(Config_www_basicauth_enabled, FPSTR(INTL_BASICAUTH), true);
	add_form_input(page_content, Config_www_username, FPSTR(INTL_USER), LEN_WWW_USERNAME-1);
	add_form_input(page_content, Config_www_password, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);

	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	// Debug Level (tab 2)

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_DEBUG "</h3>");
	add_form_input(page_content, Config_debug, FPSTR(INTL_DEBUG_LEVEL), 1);
	add_form_input(page_content, Config_sending_intervall_ms, FPSTR(INTL_MEASUREMENT_INTERVAL), 5);
	if (LED_PIN != -1) {
		add_form_checkbox(Config_leds_on, FPSTR(INTL_LEDS_ON), true);
		add_form_input(page_content, Config_leds_brightness, FPSTR(INTL_LEDS_BRIGHTNESS), 5);
	}
	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	// Firmware Version (tab 2)

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_FIRMWARE "</h3>");
	add_form_checkbox(Config_auto_update, FPSTR(INTL_AUTO_UPDATE), true);
	add_form_checkbox(Config_use_beta, FPSTR(INTL_USE_BETA), true);

	page_content += form_select_lang();

	page_content += form_select_timezone();

	// page_content += form_select_reg();

	page_content += F("<script>"
	    "var $ = function(e) { return document.getElementById(e); };"
	    "function updateOTAOptions() { "
		"$('current_lang').disabled = $('use_beta').disabled = !$('auto_update').checked; "
		"}; updateOTAOptions(); $('auto_update').onchange = updateOTAOptions;"
		"</script>");

	page_content += "</div>";

	server.sendContent(page_content);
	page_content = emptyString;

	// WiFi Sensor in configuration mode (tab 2)

	page_content = F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_WIFI_CONFIG "</h3>");
	add_form_input(page_content, Config_fs_ssid, FPSTR(INTL_FS_WIFI_NAME), LEN_FS_SSID-1);
	add_form_input(page_content, Config_fs_pwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(3));

	// Custom API (tab 3)

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_CUSTOMAPI " (BETA)</h3>");
	page_content += form_checkbox(Config_send2custom, FPSTR(INTL_SEND_TO_OWN_API), false, true);
	// page_content += form_checkbox(Config_ssl_custom, FPSTR(WEB_HTTPS), false, false);

	add_form_input(page_content, Config_host_custom, FPSTR(INTL_SERVER), LEN_HOST_CUSTOM-1, true);
	add_form_input(page_content, Config_url_custom, FPSTR(INTL_PATH), LEN_URL_CUSTOM-1, true);
	add_form_input(page_content, Config_port_custom, FPSTR(INTL_PORT), MAX_PORT_DIGITS, true);
	// add_form_input(page_content, Config_user_custom, FPSTR(INTL_USER), LEN_USER_CUSTOM-1, false);
	// add_form_input(page_content, Config_pwd_custom, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1, false);
	page_content += F("</div>");
	server.sendContent(page_content);

	// Influx DB (tab 3)
	
	page_content = F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_INFLUX " (UNDO DEVELOPMENT)</h3>");
	page_content += form_checkbox(Config_send2influx, tmpl(FPSTR(INTL_SEND_TO), F("InfluxDB")), false, false);

	page_content += form_checkbox(Config_ssl_influx, FPSTR(WEB_HTTPS), false, false);
	add_form_input(page_content, Config_host_influx, FPSTR(INTL_SERVER), LEN_HOST_INFLUX-1, false);
	add_form_input(page_content, Config_url_influx, FPSTR(INTL_PATH), LEN_URL_INFLUX-1, false);
	add_form_input(page_content, Config_port_influx, FPSTR(INTL_PORT), MAX_PORT_DIGITS, false);
	add_form_input(page_content, Config_user_influx, FPSTR(INTL_USER), LEN_USER_INFLUX-1, false);
	add_form_input(page_content, Config_pwd_influx, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1, false);
	add_form_input(page_content, Config_measurement_name_influx, FPSTR(INTL_MEASUREMENT), LEN_MEASUREMENT_NAME_INFLUX-1, false);
	page_content += "</div>";

	server.sendContent(page_content);
	page_content = emptyString;

	// CSV (tab 3)

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_CSV " (UNDO DEVELOPMENT)</h3>");
	add_form_checkbox(Config_send2csv, FPSTR(WEB_CSV), false);
	page_content += F("</div>");

	page_content += F("</span></div>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(WEB_BR_FORM);
	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
	}

	server.sendContent(page_content);
	page_content = emptyString;
}



