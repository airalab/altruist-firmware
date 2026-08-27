#include "pages.h"
#include "../../config_manager/config_helpers.h"
#include "../../utils.h"
#include "../html-content.h"
#include "../utils.h"
#include <strings.h>

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
	const String prev_reg(cfg::current_reg);

	for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());
		if (!server.hasArg(s_param)) {
			continue;
		}
		const String server_arg(server.arg(s_param));

		switch (c.cfg_type) {
		case Config_Type_UInt: {
			long v = server_arg.toInt();
			if (v < 0) {
				v = 0;
			}
			*(c.cfg_val.as_uint) = static_cast<unsigned int>(v);
			break;
		}
		case Config_Type_Time: {
			long v = server_arg.toInt();
			if (v < 0) {
				v = 0;
			}
			*(c.cfg_val.as_uint) = static_cast<unsigned int>(v) * 1000U;
			break;
		}
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

	// Region changed in UI → stop OTA from overwriting it.
	if (server.hasArg("current_reg") && prev_reg != String(cfg::current_reg)) {
		cfg::region_manual = true;
	}
	if (strcmp(cfg::current_reg, REGION_RU) != 0 && strcmp(cfg::current_reg, REGION_GLOBAL) != 0) {
		strncpy(cfg::current_reg, REGION_GLOBAL, sizeof(cfg::current_reg) - 1);
		cfg::current_reg[sizeof(cfg::current_reg) - 1] = '\0';
	}
	if (!cfg::region_manual) {
		cfgApplyAutoRegion();
	}

	// LED brightness is a percent; reject negatives / out-of-range posts.
	if (cfg::leds_brightness > 100) {
		cfg::leds_brightness = 100;
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
	if (cfg::analytics_morning_end_hour > 1439) {
		cfg::analytics_morning_end_hour = 12 * 60;
	}
	if (server.hasArg("analytics_morning_end_time")) {
		unsigned v = 0;
		if (altruistParseWebHHMM(server.arg("analytics_morning_end_time"), &v)) {
			cfg::analytics_morning_end_hour = v;
		}
	}
	if (cfg::analytics_morning_end_hour <= 6 * 60) {
		cfg::analytics_morning_end_hour = 12 * 60;
	}
#endif
	// Climate chart on sensors.map shows temperature + humidity together.
	cfg::encrypt_humidity = cfg::encrypt_temperature;
}

/*****************************************************************
 * Webserver config: show config page                            *
 *****************************************************************/

void webserver_config_send_body_get(WebServer &server, String& page_content, bool wificonfig_loop, JsonDocument &data,
                                    const char* hub_form_action, uint16_t hub_sections, const char* sensor_ss58) {
	const char *kRobonomicsNodePolkadot = "polkadot.rpc.robonomics.network";
	const char *kRobonomicsNodeKusama = "kusama.rpc.robonomics.network";
	const bool hub_mode = hub_sections != 0;
	const bool hub_wrap = hub_form_action != nullptr;
	auto section_enabled = [&](uint16_t mask) -> bool {
		return !hub_mode || ((hub_sections & mask) != 0);
	};
	auto flush_chunk = [&]() {
		web_page_flush_chunk(page_content, &server);
	};
	/** Hub sidebar anchors: only emit id= when rendering a hub zone card. */
	auto append_section_anchor = [&](const char* section_id) {
		if (hub_mode && section_id != nullptr && section_id[0] != '\0') {
			page_content += F(" id='");
			page_content += section_id;
			page_content += F("'");
		}
	};
	auto maybe_flush = [&]() {
		if (page_content.length() >= 384) {
			flush_chunk();
		}
	};
	auto add_form_checkbox = [&page_content](const ConfigShapeId cfgid, const String& info, bool enabled) {
		page_content += form_checkbox(cfgid, info, true, enabled);
	};
	auto add_form_checkbox_grid = [&page_content](const ConfigShapeId cfgid, const String& info, bool enabled) {
		// Checkbox grids use their own layout; avoid trailing <br/> which breaks grid flow.
		page_content += form_checkbox(cfgid, info, false, enabled);
	};

	debug_outln_info(F("begin webserver_config_body_get ..."));
	if (!hub_mode) {
		append_app_page_body_start(page_content, F(INTL_CONFIG_PANEL1_INTRO));
		page_content += F("<form method='POST' action='/config' class='config-form'>\n"
			"<div class='config-layout'>"
			"<aside class='config-sidebar'><nav class='config-nav tabs'>"
			"<div class='tab' data-id='1'>" INTL_COMMON_SETTINGS "</div>"
			"<div class='tab' data-id='2'>");
		page_content += FPSTR(INTL_MORE_SETTINGS);
		page_content += F("</div>"
			"<div class='tab' data-id='3'>" INTL_CONFIG_TAB_INTEGRATIONS "</div>"
			"</nav></aside>"
			"<div class='config-main'><div class='config-panels'>"
			"<div class='panel' id='panel1'>");
	} else if (hub_wrap) {
		page_content += F("<form method='POST' action='");
		page_content += hub_form_action;
		page_content += F("' class='config-form hub-config-form'><div class='hub-config-stack'>");
	}

	flush_chunk();


	// WiFi Settings (tab 1)
	if (section_enabled(HubSec_WiFi)) {
	if (!hub_mode) {
	page_content += F("<div class='config-row config-section--full'>");
	}
	page_content += F("<section class='config-section");
	if (hub_mode) {
		page_content += F(" config-section--full");
	}
	page_content += F("'");
	append_section_anchor("cfg-wifi");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_WIFI "</h2>"
		"<div class='config-section__body config-section__body--compact'>");
	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID-1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += form_checkbox(Config_wlannopwd, FPSTR(INTL_NO_WLAN_PWD), false);
	add_form_input(page_content, Config_local_hostname, FPSTR(INTL_LOCAL_HOSTNAME), LEN_LOCAL_HOSTNAME-1);
	page_content += F("</div></section>");
	if (!hub_mode && !section_enabled(HubSec_Robonomics)) {
		page_content += F("</div>");
	}

	maybe_flush();
	}

	// Robonomics Settings (tab 1)
	if (section_enabled(HubSec_Robonomics)) {
	if (!hub_mode && !section_enabled(HubSec_WiFi)) {
	page_content += F("<div class='config-row config-section--full'>");
	}

	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-robonomics");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_ROBONOMICS "</h2>"
		"<div class='config-section__body'>");
	if (!hub_mode) {
	// page_content += form_checkbox(Config_send2robonomics, FPSTR(WEB_ROBONOMICS), false);
	page_content += F("<p class='form-hint link-inline'><a href='/group'>");
	page_content += FPSTR(INTL_GROUP_MENU);
	page_content += F("</a></p>");
	}
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
				"<label for='robonomics_public_node_custom'>");
		page_content += FPSTR(INTL_ROBONOMICS_PUBLIC_NODE_CUSTOM);
		page_content += F("</label>"
				"<input type='text' id='robonomics_public_node_custom' name='robonomics_public_node_custom' "
				"placeholder='custom.rpc.example' maxlength='");
		page_content += String(LEN_ROBONOMICS_PUBLIC_NODE - 1);
		page_content += F("' value='");
		page_content += custom_value;
		page_content += F("'/></div>");
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
		page_content += F(">");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_MODE_AUTO);
		page_content += F("</option>"
				"<option value='preset'");
		if (is_preset) page_content += F(" selected='selected'");
		page_content += F(">");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_MODE_PRESET);
		page_content += F("</option>"
				"<option value='custom'");
		if (is_custom) page_content += F(" selected='selected'");
		page_content += F(">");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_MODE_CUSTOM);
		page_content += F("</option>"
				"<option value='pool'");
		if (is_pool) page_content += F(" selected='selected'");
		page_content += F(">");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_MODE_POOL);
		page_content += F("</option>"
			"</select>"
			"</div>");

		page_content += F("<div class='form-group' id='robonomics_connectivity_preset_wrap'>"
			"<label for='robonomics_connectivity_preset'>");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_PRESET_LABEL);
		page_content += F("</label>"
			"<select id='robonomics_connectivity_preset' name='robonomics_connectivity_preset'>"
				"<option value='connectivity.robonomics.network'");
		if (is_default0) page_content += F(" selected='selected'");
		page_content += F(">connectivity.robonomics.network (RU)</option>"
				"<option value='1.connectivity.robonomics.network'");
		if (is_default1) page_content += F(" selected='selected'");
		page_content += F(">1.connectivity.robonomics.network (Global)</option>"
				"<option value='2.connectivity.robonomics.network'");
		if (is_default2) page_content += F(" selected='selected'");
		page_content += F(">2.connectivity.robonomics.network (Global)</option>"
			"</select>"
			"</div>");

		page_content += F("<div class='form-group' id='robonomics_connectivity_host_wrap'>"
			"<label for='robonomics_connectivity_host'>");
		page_content += FPSTR(INTL_ROBONOMICS_CONNECTIVITY_CUSTOM_LABEL);
		page_content += F("</label>"
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

		page_content += F("<div id='robonomics_connectivity_hosts_hint' class='form-hint' style='display:none;'>"
			"Example pool:<br/>"
			"<code>connectivity.robonomics.network<br/>1.connectivity.robonomics.network<br/>2.connectivity.robonomics.network</code>"
			"</div>");
#ifdef ALTRUIST_PROTO_PROTOCOL
		page_content += F("<p class='form-hint'>");
		page_content += FPSTR(INTL_MAP_DUAL_HINT);
		page_content += F("</p>");
		add_form_checkbox(Config_map_send_csv, FPSTR(INTL_MAP_SEND_CSV), true);
		add_form_checkbox(Config_map_send_proto, FPSTR(INTL_MAP_SEND_PROTO), true);
		add_form_input(page_content, Config_robonomics_proto_connectivity_host, FPSTR(INTL_MAP_PROTO_HOST),
			LEN_ROBONOMICS_CONNECTIVITY_HOST - 1);
#endif
	}
#endif
	page_content += F("</div></section>");
	if (!hub_mode) {
	page_content += F("</div>");
	}

	maybe_flush();
	}

	if (section_enabled(HubSec_DataSharing)) {
	page_content += F("<section class='config-section config-section--full");
	page_content += F("'");
	append_section_anchor("cfg-publish");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_DATA_SHARING "</h2>"
		"<div class='config-section__body'>"
			"<p class='form-hint'>"
				INTL_DATA_SHARING_DISCLAIMER
			"</p>"
			"<div class='checkbox-grid'>");

	maybe_flush();

	add_form_checkbox_grid(Config_share_temperature, FPSTR(INTL_SHARE_TEMPERATURE), true);
	add_form_checkbox_grid(Config_share_humidity, FPSTR(INTL_SHARE_HUMIDITY), true);
	add_form_checkbox_grid(Config_share_pressure, FPSTR(INTL_SHARE_PRESSURE), true);
#ifdef ALTRUIST_INSIDE
	add_form_checkbox_grid(Config_share_co2, FPSTR(INTL_SHARE_CO2), true);
#endif
#ifdef ALTRUIST_URBAN
	add_form_checkbox_grid(Config_share_pm, FPSTR(INTL_SHARE_PM), true);
	add_form_checkbox_grid(Config_share_noise, FPSTR(INTL_SHARE_NOISE), true);
	page_content += F("<div class='form-hint' style='grid-column:1/-1;margin-top:8px;'>");
	page_content += F(INTL_DATA_SHARING_ADDITIONAL);
	page_content += F("</div>");
	add_form_checkbox_grid(Config_share_co, FPSTR(INTL_SHARE_CO), true);
	add_form_checkbox_grid(Config_share_radiation, FPSTR(INTL_SHARE_RADIATION), true);
	add_form_checkbox_grid(Config_share_o3, FPSTR(INTL_SHARE_O3), true);
	add_form_checkbox_grid(Config_share_no2, FPSTR(INTL_SHARE_NO2), true);
	add_form_checkbox_grid(Config_share_fast_aqi, FPSTR(INTL_SHARE_FAST_AQI), true);
	add_form_checkbox_grid(Config_share_epa_aqi, FPSTR(INTL_SHARE_EPA_AQI), true);
#endif
	page_content += F("</div></div></section>");

	maybe_flush();

	page_content += F("<section class='config-section config-section--full");
	page_content += F("'");
	append_section_anchor("cfg-encrypt");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_DATA_ENCRYPT "</h2>"
		"<div class='config-section__body'>"
			"<p class='form-hint'>"
				INTL_DATA_ENCRYPT_DISCLAIMER
			"</p>"
			"<div class='checkbox-grid'>");

	// One climate toggle (map chart = temp + humidity). Keep both flags aligned for the checkbox.
	{
		const bool encrypt_climate = cfg::encrypt_temperature || cfg::encrypt_humidity;
		cfg::encrypt_temperature = encrypt_climate;
		cfg::encrypt_humidity = encrypt_climate;
	}
	add_form_checkbox_grid(Config_encrypt_temperature, FPSTR(INTL_ENCRYPT_TEMPERATURE),
		cfg::share_temperature || cfg::share_humidity);
	add_form_checkbox_grid(Config_encrypt_pressure, FPSTR(INTL_ENCRYPT_PRESSURE), cfg::share_pressure);
#ifdef ALTRUIST_INSIGHT
	add_form_checkbox_grid(Config_encrypt_co2, FPSTR(INTL_ENCRYPT_CO2), cfg::share_co2);
#endif
#ifdef ALTRUIST_URBAN
	add_form_checkbox_grid(Config_encrypt_pm, FPSTR(INTL_ENCRYPT_PM), cfg::share_pm);
	add_form_checkbox_grid(Config_encrypt_noise, FPSTR(INTL_ENCRYPT_NOISE), cfg::share_noise);
	add_form_checkbox_grid(Config_encrypt_co, FPSTR(INTL_ENCRYPT_CO), cfg::share_co);
	add_form_checkbox_grid(Config_encrypt_radiation, FPSTR(INTL_ENCRYPT_RADIATION), true);
	add_form_checkbox_grid(Config_encrypt_o3, FPSTR(INTL_ENCRYPT_O3), cfg::share_o3);
	add_form_checkbox_grid(Config_encrypt_no2, FPSTR(INTL_ENCRYPT_NO2), cfg::share_no2);
	add_form_checkbox_grid(Config_encrypt_fast_aqi, FPSTR(INTL_ENCRYPT_FAST_AQI), cfg::share_fast_aqi);
	add_form_checkbox_grid(Config_encrypt_epa_aqi, FPSTR(INTL_ENCRYPT_EPA_AQI), cfg::share_epa_aqi);
#endif
	page_content += F("</div>");

	{
		const bool has_owner_key =
		    cfg::private_key[0] != '\0' && strcasecmp(cfg::private_key, "Not Set") != 0;
		if (has_owner_key) {
			page_content += F("<div class='encrypt-backup-hint'>"
				"<p class='form-hint'>");
			page_content += FPSTR(INTL_DATA_ENCRYPT_BACKUP_HINT);
			page_content += F("</p>"
				"<a class='encrypt-key-btn encrypt-key-btn--ghost' href='/advanced#backup'>");
			page_content += FPSTR(INTL_DATA_ENCRYPT_BACKUP_LINK);
			page_content += F("</a></div>");
		}
	}

	page_content += F("</div></section>");

	maybe_flush();
	}

	// GPS Settings (tab 1 — Common settings)
	if (section_enabled(HubSec_GPS)) {
#if !defined(ALTRUIST_URBAN_C3_LITE)
	page_content += F("<section class='config-section config-section--full config-section--gps");
#else
	page_content += F("<section class='config-section config-section--gps");
#endif
	page_content += F("'");
	append_section_anchor("cfg-gps");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_GPS "</h2>"
		"<div class='config-section__body'>");
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
	} else {
		page_content += F("<p class='form-hint'>");
		page_content += FPSTR(INTL_INSIGHT_STANDALONE);
		page_content += F("</p>");
	}
#endif
#if !defined(ALTRUIST_URBAN_C3_LITE)
	page_content += F("<div class='map-container'><div id='map'></div></div>");
	page_content += F("<span class='map-text'> <em>The marker on the map shows approximate location to make sure you have the right hemisphere</em></span>");
#endif
	page_content += F("</div></section>");

	maybe_flush();
	}

	if (!hub_mode) {
	flush_chunk();
	page_content += tmpl(FPSTR(WEB_DIV_PANEL), String(2));
	}

	if (section_enabled(HubSec_Auth)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-auth");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_AUTH "</h2>"
		"<div class='config-section__body'>");

	add_form_checkbox(Config_www_basicauth_enabled, FPSTR(INTL_BASICAUTH), true);
	add_form_input(page_content, Config_www_username, FPSTR(INTL_USER), LEN_WWW_USERNAME-1);
	add_form_input(page_content, Config_www_password, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);

	page_content += F("</div></section>");

	maybe_flush();
	}

	// Debug Level (tab 2)
	if (section_enabled(HubSec_Debug)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-debug");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_DEBUG "</h2>"
		"<div class='config-section__body'>");
	add_form_input(page_content, Config_debug, FPSTR(INTL_DEBUG_LEVEL), 1);
	add_form_input(page_content, Config_sending_intervall_ms, FPSTR(INTL_MEASUREMENT_INTERVAL), 5);
	page_content += F("</div></section>");
	}

	// LEDs / display (tab 2)
	if (section_enabled(HubSec_LEDs) && LED_PIN != -1) {
		page_content += F("<section class='config-section");
		page_content += F("'");
		append_section_anchor("cfg-leds");
		page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_LEDS "</h2>"
			"<div class='config-section__body'>");
		add_form_checkbox(Config_leds_on, FPSTR(INTL_LEDS_ON), true);
		page_content += F("<div class='form-fields-pack'>");
		add_form_input(page_content, Config_leds_brightness, FPSTR(INTL_LEDS_BRIGHTNESS), 5);
#ifdef ALTRUIST_INSIGHT
		add_form_input(page_content, Config_leds_off_hour, FPSTR(INTL_LEDS_OFF_HOUR), 2);
		add_form_input(page_content, Config_leds_on_hour, FPSTR(INTL_LEDS_ON_HOUR), 2);
#endif
		page_content += F("</div>");
#ifdef ALTRUIST_INSIGHT
		page_content += F("<p class='form-hint'>");
		page_content += FPSTR(INTL_LEDS_SCHEDULE_HINT);
		page_content += F("</p>");
#endif
		page_content += F("</div></section>");
	}

	maybe_flush();

#ifdef ALTRUIST_INSIDE
	// Sleep analytics (tab 2)
	if (section_enabled(HubSec_Sleep)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-sleep");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_SLEEP_ANALYTICS "</h2>"
		"<div class='config-section__body'>");
	{
		const String start_v =
		    altruistFormatHHMM(altruistNightCfgRawToMinutes(cfg::analytics_night_start_hour, 22u * 60u));
		const String end_v =
		    altruistFormatHHMM(altruistNightCfgRawToMinutes(cfg::analytics_night_end_hour, 7u * 60u));
		page_content += F("<div class='config-cluster'><h3 class='config-cluster__title'>");
		page_content += FPSTR(INTL_ANALYTICS_GROUP_NIGHT);
		page_content += F("</h3><div class='form-fields-pack'>"
			"<div class='form-group'><label for='analytics_night_start_time'>");
		page_content += FPSTR(INTL_ANALYTICS_NIGHT_START_TIME);
		page_content += F("</label>"
			"<input type='text' class='input-narrow' id='analytics_night_start_time' name='analytics_night_start_time' "
			"inputmode='numeric' pattern='^([01]?[0-9]|2[0-3]):[0-5][0-9]$' maxlength='5' placeholder='22:00' value='");
		page_content += start_v;
		page_content += F("'/></div><div class='form-group'><label for='analytics_night_end_time'>");
		page_content += FPSTR(INTL_ANALYTICS_NIGHT_END_TIME);
		page_content += F("</label>"
			"<input type='text' class='input-narrow' id='analytics_night_end_time' name='analytics_night_end_time' "
			"inputmode='numeric' pattern='^([01]?[0-9]|2[0-3]):[0-5][0-9]$' maxlength='5' placeholder='07:00' value='");
		page_content += end_v;
		page_content += F("'/></div></div><p class='form-hint'>");
		page_content += FPSTR(INTL_ANALYTICS_NIGHT_END_HINT);
		page_content += F("</p></div><div class='config-cluster'><h3 class='config-cluster__title'>");
		page_content += FPSTR(INTL_ANALYTICS_GROUP_MORNING);
		page_content += F("</h3>");
		add_form_checkbox(Config_analytics_morning_autoswitch, FPSTR(INTL_ANALYTICS_MORNING_AUTOSWITCH), true);
		{
			const String morning_end_v =
			    altruistFormatHHMM(cfgMinutesOfDay(cfg::analytics_morning_end_hour, 12u * 60u));
			page_content += F("<div class='form-group' id='analytics_morning_end_wrap'><label for='analytics_morning_end_time'>");
			page_content += FPSTR(INTL_ANALYTICS_MORNING_END_TIME);
			page_content += F("</label>"
				"<input type='text' class='input-narrow' id='analytics_morning_end_time' name='analytics_morning_end_time' "
				"inputmode='numeric' pattern='^([01]?[0-9]|2[0-3]):[0-5][0-9]$' maxlength='5' placeholder='12:00' value='");
			page_content += morning_end_v;
			page_content += F("'/><p class='form-hint'>");
			page_content += FPSTR(INTL_ANALYTICS_MORNING_END_HINT);
			page_content += F("</p></div>");
		}
		page_content += F("</div><div class='config-cluster'><h3 class='config-cluster__title'>");
		page_content += FPSTR(INTL_ANALYTICS_GROUP_DATA);
		page_content += F("</h3>");
		add_form_checkbox(Config_analytics_sleep_add_urban,
		                  FPSTR(INTL_ANALYTICS_SLEEP_ADD_URBAN),
		                  !cfg::standalone);
		if (cfg::standalone) {
			page_content += F("<p class='form-hint'>");
			page_content += FPSTR(INTL_ANALYTICS_SLEEP_ADD_URBAN_STANDALONE_HINT);
			page_content += F("</p>");
		}
		page_content += F("</div>");
	}
	page_content += F("</div></section>");

	maybe_flush();
	}
#endif

	// System (tab 2): auto-update, timezone, region
	if (section_enabled(HubSec_Firmware)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-system");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_FIRMWARE "</h2>"
		"<div class='config-section__body'>"
		"<div class='config-cluster'>");
	add_form_checkbox(Config_auto_update, FPSTR(INTL_AUTO_UPDATE), true);
#if defined(CONFIG_IDF_TARGET_ESP32C6) && defined(ALTRUIST_URBAN)
	add_form_checkbox(Config_lora_uart_enabled, FPSTR(INTL_LORA_UART_ENABLED), true);
	add_form_input(page_content, Config_lora_uart_sending_intervall_ms, FPSTR(INTL_LORA_UART_INTERVAL), 5);
	page_content += F("<p class='form-hint'>");
	page_content += FPSTR(INTL_LORA_UART_HINT);
	page_content += F("</p>");
#endif
#ifdef ALTRUIST_INSIGHT
	add_form_checkbox(Config_standalone, FPSTR(INTL_INSIGHT_STANDALONE), true);
#endif
	page_content += F("</div><div class='config-cluster'>");
	page_content += form_select_timezone();
	page_content += form_select_reg();
	page_content += F("</div></div></section>");

	maybe_flush();
	}

	// WiFi Sensor in configuration mode (tab 2)
	if (section_enabled(HubSec_WiFiConfig)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-wifi-config");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_WIFI_CONFIG "</h2>"
		"<div class='config-section__body'>");
	add_form_input(page_content, Config_fs_ssid, FPSTR(INTL_FS_WIFI_NAME), LEN_FS_SSID-1);
	add_form_input(page_content, Config_fs_pwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += F("</div></section>");

	maybe_flush();
	}

	if (!hub_mode) {
	flush_chunk();
	page_content += tmpl(FPSTR(WEB_DIV_PANEL), String(3));
	}

	// Custom API (tab 3)
	if (section_enabled(HubSec_CustomAPI)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-custom-api");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_CUSTOMAPI
		" <span class='config-tag'>" INTL_BADGE_BETA "</span></h2>"
		"<div class='config-section__body'>");
	page_content += form_checkbox(Config_send2custom, FPSTR(INTL_SEND_TO_OWN_API), false, true);
	add_form_input(page_content, Config_host_custom, FPSTR(INTL_SERVER), LEN_HOST_CUSTOM-1, true);
	add_form_input(page_content, Config_url_custom, FPSTR(INTL_PATH), LEN_URL_CUSTOM-1, true);
	add_form_input(page_content, Config_port_custom, FPSTR(INTL_PORT), MAX_PORT_DIGITS, true);
	page_content += F("</div></section>");
	maybe_flush();
	}

	// Influx DB (tab 3)
	if (section_enabled(HubSec_Influx)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-influx");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_INFLUX
		" <span class='config-tag config-tag--muted'>" INTL_BADGE_EXPERIMENTAL "</span></h2>"
		"<div class='config-section__body'>");
	page_content += form_checkbox(Config_send2influx, tmpl(FPSTR(INTL_SEND_TO), F("InfluxDB")), false, false);
	page_content += form_checkbox(Config_ssl_influx, FPSTR(WEB_HTTPS), false, false);
	add_form_input(page_content, Config_host_influx, FPSTR(INTL_SERVER), LEN_HOST_INFLUX-1, false);
	add_form_input(page_content, Config_url_influx, FPSTR(INTL_PATH), LEN_URL_INFLUX-1, false);
	add_form_input(page_content, Config_port_influx, FPSTR(INTL_PORT), MAX_PORT_DIGITS, false);
	add_form_input(page_content, Config_user_influx, FPSTR(INTL_USER), LEN_USER_INFLUX-1, false);
	add_form_input(page_content, Config_pwd_influx, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1, false);
	add_form_input(page_content, Config_measurement_name_influx, FPSTR(INTL_MEASUREMENT), LEN_MEASUREMENT_NAME_INFLUX-1, false);
	page_content += F("</div></section>");

	maybe_flush();
	}

	// CSV (tab 3)
	if (section_enabled(HubSec_CSV)) {
	page_content += F("<section class='config-section");
	page_content += F("'");
	append_section_anchor("cfg-csv");
	page_content += F("><h2 class='config-section__title'>" INTL_PANEL_TITLE_CSV
		" <span class='config-tag config-tag--muted'>" INTL_BADGE_EXPERIMENTAL "</span></h2>"
		"<div class='config-section__body'>");
	add_form_checkbox(Config_send2csv, FPSTR(WEB_CSV), false);
	page_content += F("</div></section>");
	}

	if (hub_wrap) {
		page_content += F("</div><div class='config-form-footer hub-config-footer'>");
		page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
		page_content += F("</div></form>");
		if (section_enabled(HubSec_WiFi) && wificonfig_loop) {
			page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
		}
	} else if (!hub_mode) {
		page_content += F("</div></div><div class='config-form-footer'>");
		page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
		page_content += F("</div></div></div>");
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(WEB_BR_FORM);
		if (wificonfig_loop) {  // scan for wlan ssids
			page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
		}

		append_app_page_body_end(page_content);
	}
	flush_chunk();
}

