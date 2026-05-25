#include "pages.h"
#include "../../utils.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"

/*****************************************************************
 * Webserver config: show guest page                            *
 *****************************************************************/

void webserver_guest_create_body_get_part1(String& page_content, bool wificonfig_loop, device_status_t &deviceStatus) {

	debug_outln_info(F("begin webserver_config_body_get ..."));
	page_content += F("<form class='guest-form' method='POST' action='/guest' style='width:100%;'>\n");

	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += F("<div id='wifilist'>" INTL_WIFI_NETWORKS "</div><br/>");
	}

	page_content += F("<h3 class='guest-subtitle'>");
	page_content += F("Wifi Settings");
	page_content += F("</h3>");

	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID-1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += form_checkbox(Config_wlannopwd, FPSTR(INTL_NO_WLAN_PWD), false);
	add_form_input(page_content, Config_local_hostname, FPSTR(INTL_LOCAL_HOSTNAME), LEN_LOCAL_HOSTNAME-1);

	page_content += form_select_timezone();
	
	// page_content += F("<h3 class='guest-subtitle'>");
	// page_content += F("Robonomics Settings");
	// page_content += F("</h3>");
	// add_form_input(page_content, Config_rws_owner, FPSTR(INTL_RWS_OWNER), LEN_RWS_OWNER-1);
	// page_content += form_select_reg();

	// page_content += F("<h3 class='guest-subtitle'>");
	// page_content += F("GPS Settings");
	// page_content += F("</h3>");
	// add_form_input(page_content, Config_coords_gps, FPSTR(INTL_COORDS), LEN_GPS_COORDS-1);
	// page_content += F("<div class='map-container  map-container--guest'><div id='map'></div>");
	// page_content += F("</div><span class='map-text'> <em>The marker on the map shows approximate location to make sure you have the right hemisphere</em></span>");

	page_content += F("<h3 class='guest-subtitle'>");
	page_content += F("Connected Sensors");
	page_content += F("</h3>");

	page_content += F("<ul class='list'>");

	for (const auto &sensor : deviceStatus.sensor_names) {
		page_content += F("<li class='list-item'>");
		page_content += sensor.c_str();
		page_content += F("</li>");
    }
	page_content += F("</ul>");

	page_content += F(
		"<div style='margin-top:30px;padding:16px 20px;border:2px solid #2949d3;border-radius:10px;background:#f0f4ff;'>"
		"<h3 class='guest-subtitle' style='margin-bottom:12px;'>" INTL_PANEL_TITLE_DATA_SHARING "</h3>"
		"<p style='font-size:14px;color:#333;margin:0 0 14px;line-height:1.6;'>"
		"&#9432;&nbsp;" INTL_DATA_SHARING_DISCLAIMER
		"</p>"
		"<div style='padding:8px 0;'>");
	page_content += form_checkbox(Config_share_temperature, FPSTR(INTL_SHARE_TEMPERATURE), false);
	page_content += form_checkbox(Config_share_humidity, FPSTR(INTL_SHARE_HUMIDITY), false);
	page_content += form_checkbox(Config_share_pressure, FPSTR(INTL_SHARE_PRESSURE), false);
#ifdef ALTRUIST_INSIDE
	page_content += form_checkbox(Config_share_co2, FPSTR(INTL_SHARE_CO2), false);
#endif
#ifdef ALTRUIST_URBAN
	page_content += form_checkbox(Config_share_pm, FPSTR(INTL_SHARE_PM), false);
	page_content += form_checkbox(Config_share_noise, FPSTR(INTL_SHARE_NOISE), false);
	page_content += F("<div style='margin-top:14px;margin-bottom:14px;font-size:14px;color:#555;'>");
	page_content += F(INTL_DATA_SHARING_ADDITIONAL);
	page_content += F("</div>");
	page_content += form_checkbox(Config_share_co, FPSTR(INTL_SHARE_CO), false);
	page_content += form_checkbox(Config_share_radiation, FPSTR(INTL_SHARE_RADIATION), false);
	page_content += form_checkbox(Config_share_o3, FPSTR(INTL_SHARE_O3), false);
	page_content += form_checkbox(Config_share_no2, FPSTR(INTL_SHARE_NO2), false);
	page_content += form_checkbox(Config_share_fast_aqi, FPSTR(INTL_SHARE_FAST_AQI), false);
	page_content += form_checkbox(Config_share_epa_aqi, FPSTR(INTL_SHARE_EPA_AQI), false);
#endif
	page_content += F("</div></div>");
}

void webserver_guest_create_body_get_part2(String& page_content, bool wificonfig_loop) {
	page_content += F("</div></div>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(WEB_BR_FORM);
	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
	}
}
