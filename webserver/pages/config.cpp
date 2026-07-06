#include "pages.h"
#include "../../config_manager/config_helpers.h"
#include "../../utils.h"
#include "../html-content.h"
#include "../utils.h"

#ifdef ALTRUIST_INSIGHT
/** Parse "H:MM" or "HH:MM" (24h) into minutes since midnight [0..1439]. */
static bool altruistParseWebHHMM(const String &raw, unsigned *out_min) {
	if (!out_min) {
		return false;
	}
	String s = raw;
	s.trim();
	const int colon = s.indexOf(':');
	if (colon <= 0) {
		return false;
	}
	String hs = s.substring(0, colon);
	String ms = s.substring(colon + 1);
	ms.trim();
	if (ms.length() == 0) {
		return false;
	}
	const int h = hs.toInt();
	const int m = ms.toInt();
	if (h < 0 || h > 23 || m < 0 || m > 59) {
		return false;
	}
	*out_min = (unsigned)(h * 60 + m);
	return true;
}

static String altruistFormatHHMM(unsigned minutes) {
	if (minutes > 1439u) {
		minutes = 0;
	}
	char b[8];
	snprintf(b, sizeof(b), "%02u:%02u", (unsigned)(minutes / 60U), (unsigned)(minutes % 60U));
	return String(b);
}

/** Raw NVS value: legacy 0..23 = whole hour; else minutes 0..1439. */
static unsigned altruistNightCfgRawToMinutes(unsigned raw, unsigned fallback_minutes) {
	if (raw <= 23u) {
		return raw * 60u;
	}
	if (raw > 1439u) {
		return fallback_minutes;
	}
	return raw;
}
#endif

void webserver_config_send_body_post(WebServer &server) {
	String masked_pwd;
	const char *kRobonomicsNodePolkadot = "polkadot.rpc.robonomics.network";
	const char *kRobonomicsNodeKusama = "kusama.rpc.robonomics.network";

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

	// Robonomics public node can be selected from presets or entered manually.
	if (server.hasArg("robonomics_public_node_select")) {
		const String selected = server.arg("robonomics_public_node_select");
		if (selected == "custom") {
			if (server.hasArg("robonomics_public_node_custom")) {
				String custom = server.arg("robonomics_public_node_custom");
				custom.trim();
				if (custom.length() > 0) {
					strncpy(cfg::robonomics_public_node, custom.c_str(), LEN_ROBONOMICS_PUBLIC_NODE - 1);
					cfg::robonomics_public_node[LEN_ROBONOMICS_PUBLIC_NODE - 1] = '\0';
				}
			}
		} else if (selected == kRobonomicsNodePolkadot || selected == kRobonomicsNodeKusama) {
			strncpy(cfg::robonomics_public_node, selected.c_str(), LEN_ROBONOMICS_PUBLIC_NODE - 1);
			cfg::robonomics_public_node[LEN_ROBONOMICS_PUBLIC_NODE - 1] = '\0';
		}
	}

	// Robonomics Map (connectivity) hosts:
	// Modes:
	// - auto: use built-in HOST_ROBONOMICS pool (clear both config values)
	// - preset: pinned to one of built-in hosts (store in robonomics_connectivity_host)
	// - custom: pinned custom host (store in robonomics_connectivity_host)
	// - pool: custom host pool list (store in robonomics_connectivity_hosts)
	//
	// Important: only update when selector is present, so saving other tabs doesn't clobber.
	if (server.hasArg("robonomics_connectivity_mode")) {
		const String mode = server.arg("robonomics_connectivity_mode");
		if (mode == "auto") {
			cfg::robonomics_connectivity_host[0] = '\0';
			cfg::robonomics_connectivity_hosts[0] = '\0';
		} else if (mode == "pool") {
			cfg::robonomics_connectivity_host[0] = '\0';
			if (server.hasArg("robonomics_connectivity_hosts")) {
				String v = server.arg("robonomics_connectivity_hosts");
				v.trim();
				strncpy(cfg::robonomics_connectivity_hosts, v.c_str(), LEN_ROBONOMICS_CONNECTIVITY_HOSTS - 1);
				cfg::robonomics_connectivity_hosts[LEN_ROBONOMICS_CONNECTIVITY_HOSTS - 1] = '\0';
			}
		} else if (mode == "preset") {
			cfg::robonomics_connectivity_hosts[0] = '\0';
			if (server.hasArg("robonomics_connectivity_preset")) {
				String v = server.arg("robonomics_connectivity_preset");
				v.trim();
				strncpy(cfg::robonomics_connectivity_host, v.c_str(), LEN_ROBONOMICS_CONNECTIVITY_HOST - 1);
				cfg::robonomics_connectivity_host[LEN_ROBONOMICS_CONNECTIVITY_HOST - 1] = '\0';
			}
		} else { // "custom"
			cfg::robonomics_connectivity_hosts[0] = '\0';
			if (server.hasArg("robonomics_connectivity_host")) {
				String v = server.arg("robonomics_connectivity_host");
				v.trim();
				strncpy(cfg::robonomics_connectivity_host, v.c_str(), LEN_ROBONOMICS_CONNECTIVITY_HOST - 1);
				cfg::robonomics_connectivity_host[LEN_ROBONOMICS_CONNECTIVITY_HOST - 1] = '\0';
			}
		}
	}

#ifdef ALTRUIST_INSIGHT
	// Keep LED schedule values in a safe 0..23 range even for crafted requests.
	if (cfg::leds_off_hour > 23) cfg::leds_off_hour = 0;
	if (cfg::leds_on_hour > 23) cfg::leds_on_hour = 6;
	// Sleep analytics window: minutes [0..1439] (also clamp UInt posts).
	if (cfg::analytics_night_start_hour > 1439) cfg::analytics_night_start_hour = 22 * 60;
	if (cfg::analytics_night_end_hour > 1439) cfg::analytics_night_end_hour = 7 * 60;
	if (server.hasArg("analytics_night_start_time")) {
		unsigned v = 0;
		if (altruistParseWebHHMM(server.arg("analytics_night_start_time"), &v)) {
			cfg::analytics_night_start_hour = v;
		}
	}
	if (server.hasArg("analytics_night_end_time")) {
		unsigned v = 0;
		if (altruistParseWebHHMM(server.arg("analytics_night_end_time"), &v)) {
			cfg::analytics_night_end_hour = v;
		}
	}
#endif
}

/*****************************************************************
 * Webserver config: show config page                            *
 *****************************************************************/

void webserver_config_send_body_get(WebServer &server, String& page_content, bool wificonfig_loop, JsonDocument &data) {
	const char *kRobonomicsNodePolkadot = "polkadot.rpc.robonomics.network";
	const char *kRobonomicsNodeKusama = "kusama.rpc.robonomics.network";
	auto add_form_checkbox = [&page_content](const ConfigShapeId cfgid, const String& info, bool enabled) {
		page_content += form_checkbox(cfgid, info, true, enabled);
	};

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
	page_content += F("<p style='font-size:12px;margin:0 0 8px 0;'><a href='/group'>");
	page_content += FPSTR(INTL_GROUP_MENU);
	page_content += F("</a></p>");
	add_form_input(page_content, Config_rws_owner, FPSTR(INTL_RWS_OWNER), LEN_RWS_OWNER-1);
	add_form_input(page_content, Config_datalog_sending_intervall_ms, FPSTR(INTL_DATALOG_SENDING_INTERVAL), 5);
	{
		String current_node = String(cfg::robonomics_public_node);
		const bool is_polkadot = current_node == kRobonomicsNodePolkadot;
		const bool is_kusama = current_node == kRobonomicsNodeKusama;
		const bool is_custom = !is_polkadot && !is_kusama;
		String custom_value = is_custom ? current_node : String("");
		custom_value.replace("'", "&#39;");

		page_content += F("<div class='form-group'>"
				"<label for='robonomics_public_node_select'>");
		page_content += FPSTR(INTL_ROBONOMICS_PUBLIC_NODE);
		page_content += F("</label>"
				"<select id='robonomics_public_node_select' name='robonomics_public_node_select'>"
					"<option value='polkadot.rpc.robonomics.network'");
		if (is_polkadot) page_content += F(" selected='selected'");
		page_content += F(">polkadot.rpc.robonomics.network</option>"
					"<option value='kusama.rpc.robonomics.network'");
		if (is_kusama) page_content += F(" selected='selected'");
		page_content += F(">kusama.rpc.robonomics.network</option>"
					"<option value='custom'");
		if (is_custom) page_content += F(" selected='selected'");
		page_content += F(">Custom</option>"
				"</select>"
				"</div>");

		page_content += F("<div class='form-group' id='robonomics_public_node_custom_wrap'>"
				"<label for='robonomics_public_node_custom'>Custom Robonomics Public Node</label>"
				"<input type='text' id='robonomics_public_node_custom' name='robonomics_public_node_custom' "
				"placeholder='custom.rpc.example' maxlength='");
		page_content += String(LEN_ROBONOMICS_PUBLIC_NODE - 1);
		page_content += F("' value='");
		page_content += custom_value;
		page_content += F("'/></div>");

		page_content += F("<script>"
				"(function(){"
					"var sel=document.getElementById('robonomics_public_node_select');"
					"var wrap=document.getElementById('robonomics_public_node_custom_wrap');"
					"var input=document.getElementById('robonomics_public_node_custom');"
					"function sync(){"
						"var custom=(sel && sel.value==='custom');"
						"if(wrap){wrap.style.display=custom?'block':'none';}"
						"if(input){input.disabled=!custom;}"
					"}"
					"sync();"
					"if(sel){sel.onchange=sync;}"
				"})();"
				"</script>");
	}

#if !defined(ALTRUIST_URBAN_C3_LITE)
	// Robonomics Map (connectivity) custom hosts.
	{
		String pinned = String(cfg::robonomics_connectivity_host);
		pinned.trim();
		pinned.replace("'", "&#39;");
		String pool = String(cfg::robonomics_connectivity_hosts);
		pool.trim();
		pool.replace("'", "&#39;");

		const bool is_auto = (pinned.length() == 0 && pool.length() == 0);
		const bool is_pool = (pinned.length() == 0 && pool.length() > 0);
		const bool is_default0 = pinned == "connectivity.robonomics.network";
		const bool is_default1 = pinned == "1.connectivity.robonomics.network";
		const bool is_default2 = pinned == "2.connectivity.robonomics.network";
		const bool is_preset = (pinned.length() > 0 && (is_default0 || is_default1 || is_default2));
		const bool is_custom = (pinned.length() > 0 && !is_preset);

		page_content += F("<div class='form-group'>"
			"<label for='robonomics_connectivity_mode'>");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_HOST);
		page_content += F("</label>"
			"<select id='robonomics_connectivity_mode' name='robonomics_connectivity_mode'>"
				"<option value='auto'");
		if (is_auto) page_content += F(" selected='selected'");
		page_content += F(">Default pool (auto)</option>"
				"<option value='preset'");
		if (is_preset) page_content += F(" selected='selected'");
		page_content += F(">Pinned (preset)</option>"
				"<option value='custom'");
		if (is_custom) page_content += F(" selected='selected'");
		page_content += F(">Custom host</option>"
				"<option value='pool'");
		if (is_pool) page_content += F(" selected='selected'");
		page_content += F(">Custom pool</option>"
			"</select>"
			"</div>");

		page_content += F("<div class='form-group' id='robonomics_connectivity_preset_wrap'>"
			"<label for='robonomics_connectivity_preset'>Pinned host (preset)</label>"
			"<select id='robonomics_connectivity_preset' name='robonomics_connectivity_preset'>"
				"<option value='connectivity.robonomics.network'");
		if (is_default0) page_content += F(" selected='selected'");
		page_content += F(">connectivity.robonomics.network</option>"
				"<option value='1.connectivity.robonomics.network'");
		if (is_default1) page_content += F(" selected='selected'");
		page_content += F(">1.connectivity.robonomics.network</option>"
				"<option value='2.connectivity.robonomics.network'");
		if (is_default2) page_content += F(" selected='selected'");
		page_content += F(">2.connectivity.robonomics.network</option>"
			"</select>"
			"</div>");

		page_content += F("<div class='form-group' id='robonomics_connectivity_host_wrap'>"
			"<label for='robonomics_connectivity_host'>Custom host</label>"
			"<input type='text' id='robonomics_connectivity_host' name='robonomics_connectivity_host' "
			"placeholder='custom.connectivity.example' maxlength='");
		page_content += String(LEN_ROBONOMICS_CONNECTIVITY_HOST - 1);
		page_content += F("' value='");
		page_content += (is_custom ? pinned : String(""));
		page_content += F("'/></div>");

		page_content += F("<div class='form-group'>"
			"<label for='robonomics_connectivity_hosts'>");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_HOSTS);
		page_content += F("</label>"
			"<textarea id='robonomics_connectivity_hosts' name='robonomics_connectivity_hosts' rows='4' "
			"placeholder='Example:&#10;connectivity.robonomics.network&#10;1.connectivity.robonomics.network&#10;2.connectivity.robonomics.network' maxlength='");
		page_content += String(LEN_ROBONOMICS_CONNECTIVITY_HOSTS - 1);
		page_content += F("'>");
		page_content += (is_pool ? pool : String(""));
		page_content += F("</textarea></div>");

		page_content += F("<div id='robonomics_connectivity_hosts_hint' style='font-size:12px;color:#666;margin-top:-6px;margin-bottom:10px;display:none;'>"
			"Example pool:<br/>"
			"<code>connectivity.robonomics.network<br/>1.connectivity.robonomics.network<br/>2.connectivity.robonomics.network</code>"
			"</div>");

		page_content += F("<script>"
			"(function(){"
				"var mode=document.getElementById('robonomics_connectivity_mode');"
				"var wPreset=document.getElementById('robonomics_connectivity_preset_wrap');"
				"var wHost=document.getElementById('robonomics_connectivity_host_wrap');"
				"var ta=document.getElementById('robonomics_connectivity_hosts');"
				"var hint=document.getElementById('robonomics_connectivity_hosts_hint');"
				"function sync(){"
					"var m=mode?mode.value:'auto';"
					"if(wPreset) wPreset.style.display=(m==='preset')?'block':'none';"
					"if(wHost) wHost.style.display=(m==='custom')?'block':'none';"
					"if(ta) ta.disabled=(m!=='pool');"
					"var wTa=ta?ta.parentElement:null;"
					"if(wTa) wTa.style.display=(m==='pool')?'block':'none';"
					"if(hint) hint.style.display=(m==='pool')?'block':'none';"
				"}"
				"sync();"
				"if(mode) mode.onchange=sync;"
			"})();"
			"</script>");
	}
#endif

	page_content += F("<h3 class='panel-subtitle' style='margin-top:16px;'>" INTL_PANEL_TITLE_DATA_SHARING "</h3>");
	page_content += F("<p style='font-size:0.9em;color:#555;margin-bottom:12px;'>"
		INTL_DATA_SHARING_DISCLAIMER
		"</p>");

	server.sendContent(page_content);
	page_content = emptyString;

	add_form_checkbox(Config_share_temperature, FPSTR(INTL_SHARE_TEMPERATURE), true);
	add_form_checkbox(Config_share_humidity, FPSTR(INTL_SHARE_HUMIDITY), true);
	add_form_checkbox(Config_share_pressure, FPSTR(INTL_SHARE_PRESSURE), true);
#ifdef ALTRUIST_INSIGHT
	add_form_checkbox(Config_share_co2, FPSTR(INTL_SHARE_CO2), true);
#endif
#ifdef ALTRUIST_URBAN
	add_form_checkbox(Config_share_pm, FPSTR(INTL_SHARE_PM), true);
	add_form_checkbox(Config_share_noise, FPSTR(INTL_SHARE_NOISE), true);
	page_content += F("<div style='margin-top:14px;margin-bottom:14px;font-size:0.9em;color:#666;'>");
	page_content += F(INTL_DATA_SHARING_ADDITIONAL);
	page_content += F("</div>");
	add_form_checkbox(Config_share_co, FPSTR(INTL_SHARE_CO), true);
	add_form_checkbox(Config_share_radiation, FPSTR(INTL_SHARE_RADIATION), true);
	add_form_checkbox(Config_share_o3, FPSTR(INTL_SHARE_O3), true);
	add_form_checkbox(Config_share_no2, FPSTR(INTL_SHARE_NO2), true);
	add_form_checkbox(Config_share_fast_aqi, FPSTR(INTL_SHARE_FAST_AQI), true);
	add_form_checkbox(Config_share_epa_aqi, FPSTR(INTL_SHARE_EPA_AQI), true);
#endif
	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	// GPS Settings (tab 1)

#if defined(ALTRUIST_URBAN_C3_LITE)
	page_content += F("<div class='panel-container'>");
#else
	page_content += F("<div class='panel-container panel-container--with-map'>");
#endif
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_GPS "</h3>");
	add_form_input(page_content, Config_coords_gps, FPSTR(INTL_COORDS), LEN_GPS_COORDS-1);
	add_form_input(page_content, Config_temp_correction, FPSTR(INTL_TEMP_CORRECTION), LEN_TEMP_CORRECTION-1);
#ifdef ALTRUIST_URBAN
	add_form_input(page_content, Config_sds_meas_interval_ms, FPSTR(INTL_SDS_MEAS_INTERVAL), 5);
#endif
#ifdef ALTRUIST_INSIGHT
	if (!cfg::standalone) {
		page_content += form_select_altruist(data);
		add_form_checkbox(Config_use_custom_urban, FPSTR(INTL_USE_CUSTOM_URBAN), true);
		add_form_input(page_content, Config_custom_altruist_urban, FPSTR(INTL_CUSTOM_ALTRUIST), LEN_CHOSEN_ALTRUIS_ADDRESS-1);
		page_content += F("<script>"
		    "var $ = function(e) { return document.getElementById(e); };"
		    "function updateUrbanOptions() { "
			"$('custom_altruist_urban').disabled = !$('use_custom_urban').checked; "
			"$('chosen_altruist_urban').disabled = $('use_custom_urban').checked;"
			"}; updateUrbanOptions(); $('use_custom_urban').onchange = updateUrbanOptions;"
			"</script>");
	} else {
		page_content += F("<p style='font-size:14px;color:#555;margin:0 0 8px;line-height:1.5;'>");
		page_content += FPSTR(INTL_INSIGHT_STANDALONE);
		page_content += F("</p>");
	}
#endif
#if !defined(ALTRUIST_URBAN_C3_LITE)
	page_content += F("<div class='map-container'><div id='map'></div>");
	page_content += F("</div><span class='map-text'> <em>The marker on the map shows approximate location to make sure you have the right hemisphere</em></span>");
#endif
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
#ifdef ALTRUIST_INSIGHT
		add_form_input(page_content, Config_leds_off_hour, FPSTR(INTL_LEDS_OFF_HOUR), 2);
		add_form_input(page_content, Config_leds_on_hour, FPSTR(INTL_LEDS_ON_HOUR), 2);
		{
			const String start_v =
			    altruistFormatHHMM(altruistNightCfgRawToMinutes(cfg::analytics_night_start_hour, 22u * 60u));
			const String end_v =
			    altruistFormatHHMM(altruistNightCfgRawToMinutes(cfg::analytics_night_end_hour, 7u * 60u));
			page_content += F("<div class='form-group'><label for='analytics_night_start_time'>Sleep analytics night start (local, HH:MM)</label>"
				"<input type='text' id='analytics_night_start_time' name='analytics_night_start_time' "
				"inputmode='numeric' pattern='^([01]?[0-9]|2[0-3]):[0-5][0-9]$' maxlength='5' placeholder='22:00' value='");
			page_content += start_v;
			page_content += F("'/></div><div class='form-group'><label for='analytics_night_end_time'>Sleep analytics night end (local, HH:MM)</label>"
				"<input type='text' id='analytics_night_end_time' name='analytics_night_end_time' "
				"inputmode='numeric' pattern='^([01]?[0-9]|2[0-3]):[0-5][0-9]$' maxlength='5' placeholder='07:00' value='");
			page_content += end_v;
			page_content += F("'/></div><p style='font-size:12px;color:#666;margin:4px 0 0;'>End is exclusive for hourly buckets (e.g. 07:00 uses hours through 06:xx).</p>");
			add_form_checkbox(Config_analytics_sleep_add_urban,
			                  F("Add Urban data to sleep analytics (PM2.5 & noise)"),
			                  !cfg::standalone);
			if (cfg::standalone) {
				page_content += F("<p style='font-size:12px;color:#666;margin:4px 0 8px;'>"
				                  "Uncheck Insight standalone above to pair with Urban and use outdoor PM2.5/noise in sleep analytics.</p>");
			}
		}
		page_content += F("<script>"
			"(function(){"
				"var off=document.getElementById('leds_off_hour');"
				"var on=document.getElementById('leds_on_hour');"
				"if(off){off.min='0';off.max='23';off.step='1';}"
				"if(on){on.min='0';on.max='23';on.step='1';}"
			"})();"
		"</script>");
#endif
	}
	page_content += F("</div>");

	server.sendContent(page_content);
	page_content = emptyString;

	// Firmware Version (tab 2)

	page_content += F("<div class='panel-container'>");
	page_content += F("<h3 class='panel-subtitle'>" INTL_PANEL_TITLE_FIRMWARE "</h3>");
	add_form_checkbox(Config_auto_update, FPSTR(INTL_AUTO_UPDATE), true);
#ifdef ALTRUIST_INSIGHT
	add_form_checkbox(Config_standalone, FPSTR(INTL_INSIGHT_STANDALONE), true);
#endif

	page_content += form_select_lang();

	page_content += form_select_timezone();

	// page_content += form_select_reg();

	page_content += F("<script>"
	    "var $ = function(e) { return document.getElementById(e); };"
	    "function updateOTAOptions() { "
		"$('current_lang').disabled = !$('auto_update').checked; "
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

