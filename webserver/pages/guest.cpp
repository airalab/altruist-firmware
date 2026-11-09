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

	page_content += F("<div class='guest-page'><div class='guest-card'><form class='guest-form guest-wizard-form' method='POST' action='/guest'>");

#ifdef ALTRUIST_INSIGHT
	page_content += F("<div class='guest__setup-header'>"
		"<span class='guest__step-label'>" INTL_GUEST_SETUP_STEP_1_LABEL "</span>"
		"<h2 class='guest__step-title'>" INTL_GUEST_SETUP_STEP_1_TITLE "</h2>"
		"</div>");
#endif

	page_content += F("<section class='config-section'><h2 class='config-section__title'>" INTL_PANEL_TITLE_WIFI "</h2>"
		"<div class='config-section__body config-section__body--compact'>");

	if (wificonfig_loop) {
		page_content += F("<div id='wifilist' class='guest-wifilist'>");
		page_content += FPSTR(INTL_WIFI_NETWORKS);
		page_content += F("</div>");
	}

	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID - 1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	page_content += form_checkbox(Config_wlannopwd, FPSTR(INTL_NO_WLAN_PWD), false);
	add_form_input(page_content, Config_local_hostname, FPSTR(INTL_LOCAL_HOSTNAME), LEN_LOCAL_HOSTNAME - 1);
	page_content += form_select_timezone();
	page_content += form_select_reg();
	page_content += F("</div></section>");

	page_content += F("<section class='config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_GUEST_CONNECTED_SENSORS);
	page_content += F("</h2><div class='config-section__body'><ul class='guest-sensor-list'>");

	for (const auto &sensor : deviceStatus.sensor_names) {
		page_content += F("<li>");
		page_content += sensor.c_str();
		page_content += F("</li>");
	}
	page_content += F("</ul></div></section>");

	page_content += F("<section class='config-section'><h2 class='config-section__title'>" INTL_PANEL_TITLE_DATA_SHARING "</h2>"
		"<div class='config-section__body'>"
		"<p class='form-hint guest-hint'>"
		"&#9432;&nbsp;" INTL_DATA_SHARING_DISCLAIMER
		"</p>"
		"<div class='checkbox-grid'>");
	page_content += form_checkbox(Config_share_temperature, FPSTR(INTL_SHARE_TEMPERATURE), false);
	page_content += form_checkbox(Config_share_humidity, FPSTR(INTL_SHARE_HUMIDITY), false);
	page_content += form_checkbox(Config_share_pressure, FPSTR(INTL_SHARE_PRESSURE), false);
#ifdef ALTRUIST_INSIGHT
	page_content += form_checkbox(Config_share_co2, FPSTR(INTL_SHARE_CO2), false);
#endif
#ifdef ALTRUIST_URBAN
	page_content += form_checkbox(Config_share_pm, FPSTR(INTL_SHARE_PM), false);
	page_content += form_checkbox(Config_share_noise, FPSTR(INTL_SHARE_NOISE), false);
	page_content += F("</div><p class='form-hint'>");
	page_content += F(INTL_DATA_SHARING_ADDITIONAL);
	page_content += F("</p><div class='checkbox-grid'>");
	page_content += form_checkbox(Config_share_co, FPSTR(INTL_SHARE_CO), false);
	page_content += form_checkbox(Config_share_radiation, FPSTR(INTL_SHARE_RADIATION), false);
	page_content += form_checkbox(Config_share_o3, FPSTR(INTL_SHARE_O3), false);
	page_content += form_checkbox(Config_share_no2, FPSTR(INTL_SHARE_NO2), false);
	page_content += form_checkbox(Config_share_fast_aqi, FPSTR(INTL_SHARE_FAST_AQI), false);
	page_content += form_checkbox(Config_share_epa_aqi, FPSTR(INTL_SHARE_EPA_AQI), false);
#endif
	page_content += F("</div></div></section>");
}

void webserver_guest_create_body_get_part2(String& page_content, bool wificonfig_loop) {
	page_content += F("<div class='guest-form-footer'>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += F("</div></form></div></div>");

	page_content += F("<div class='guest-page guest-page--backup'><div class='guest-card'>"
		"<div class='guest__setup-header'>"
		"<h2 class='guest__step-title'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_TITLE);
	page_content += F("</h2></div>"
		"<div class='hub-backup'>");
	append_device_backup_restore_form(page_content, FPSTR(INTL_GUEST_RESTORE_HINT), true);
	page_content += F("</div></div></div>");

	if (wificonfig_loop) {
		page_content += F("<script>window.setTimeout(load_wifi_list,300);window.setTimeout(load_wifi_list,3500);</script>");
	}
	page_content += FPSTR(WEB_GUEST_WIZARD_SUBMIT_JS);
}
