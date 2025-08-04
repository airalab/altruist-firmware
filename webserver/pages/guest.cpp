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
