#include "webserver.h"
#include "pages/pages.h"
#include "html-content.h"
#include <WiFi.h>
#include "script-js.h"
#include "utils.h"
#include "../config_manager/config_helpers.h"
#include "web-header-logo-select.h"
#include "robonomics-logo-common.h"

#ifdef ALTRUIST_INSIDE
#include <ESPmDNS.h>
#endif


void SensorWebServer::setup() {
    www_username = cfg::www_username;
    www_password = cfg::www_password;
    uint64_t chipid_num;
	esp_chipid = get_chipid();

	server.on("/guest", std::bind(&SensorWebServer::_webserver_guest, this)); // x
	server.on("/", std::bind(&SensorWebServer::_webserver_root, this)); // x
	server.on(F("/config"), std::bind(&SensorWebServer::_webserver_config, this)); // x
	server.on(F("/wifi"), std::bind(&SensorWebServer::_webserver_wifi, this)); // x
	server.on(F("/values"), std::bind(&SensorWebServer::_webserver_values, this)); // x
	server.on(F("/status"), std::bind(&SensorWebServer::_webserver_status, this));
	server.on(F("/generate_204"), std::bind(&SensorWebServer::_webserver_config, this)); // x
	server.on(F("/fwlink"), std::bind(&SensorWebServer::_webserver_config, this)); // x
	server.on(F("/debug"), std::bind(&SensorWebServer::_webserver_debug_level, this)); // x
	server.on(F("/serial"), std::bind(&SensorWebServer::_webserver_serial, this)); // x
	server.on(F("/removeConfig"), std::bind(&SensorWebServer::_webserver_removeConfig, this)); // x
	server.on(F("/restart"), std::bind(&SensorWebServer::_webserver_restart, this)); // x
	server.on(F("/data.json"), std::bind(&SensorWebServer::_webserver_data_json, this)); // x
	server.on(F("/favicon.ico"), std::bind(&SensorWebServer::_webserver_favicon, this)); // x
	server.on(F(STATIC_PREFIX), std::bind(&SensorWebServer::_webserver_static, this)); // x
	server.on(F("/ota"), std::bind(&SensorWebServer::_webserver_ota, this));
#ifdef ALTRUIST_INSIDE
	server.on(F("/select_urban"), std::bind(&SensorWebServer::_webserver_select_urban, this));
	server.on(F("/scan_urbans"), std::bind(&SensorWebServer::_webserver_scan_urbans, this));
#endif
	server.onNotFound(std::bind(&SensorWebServer::_webserver_not_found, this)); // x

	debug_outln_info(F("Starting Webserver... "), WiFi.localIP().toString());
	server.begin();
}

void SensorWebServer::notifyStaIpRestored() {
#if defined(ESP32)
	// WebServer::begin() calls close() internally, but after a netif/IP change an explicit stop + short yield
	// avoids a stuck listen socket so phones/browsers can reach the device again on the LAN IP.
	debug_outln_info(F("Webserver: rebind after STA IP "), WiFi.localIP().toString());
	server.stop();
	yield();
	delay(30);
	server.begin();
#endif
}

void SensorWebServer::_webserver_status() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEVICE_STATUS));

	debug_outln_info(F("ws: status ..."));
	server.sendContent(page_content);
    webserver_status_part1(page_content, deviceStatus);
    server.sendContent(page_content);
    page_content = FPSTR(EMPTY_ROW);
    webserver_status_part2(page_content, deviceStatus);
    // server.sendContent(page_content);

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	end_html_page(page_content);
}

void SensorWebServer::_webserver_data_json() {
	String json_content;
	if (xSemaphoreTake(mutex, portMAX_DELAY)) {
    	webserver_data_json(sensors_data, esp_chipid, json_content);
		xSemaphoreGive(mutex);
	}
    server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json_content);
}

void SensorWebServer::_webserver_not_found() {
	debug_outln_info(F("ws: not found ..."));
	if (WiFi.status() != WL_CONNECTED) {
		if ((server.uri().indexOf(F("success.html")) != -1) || (server.uri().indexOf(F("detect.html")) != -1)) {
			server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), FPSTR(WEB_IOS_REDIRECT));
		} else {
			sendHttpRedirectGuest();
		}
	} else {
		server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Not found."));
	}
}

void SensorWebServer::_webserver_static() {
	if (server.arg(String('r')) == F("logo")) {
		// Device-specific SVG (Insight vs Urban build); avoid stale PNG/SVG from old Cache-Control.
		server.sendHeader(F("Cache-Control"), F("no-cache"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_SVG, WEB_HEADER_LOGO_SVG, WEB_HEADER_LOGO_SIZE);
		return;
	}
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	if (server.arg(String('r')) == F("css")) {
		server.send_P(200, TXT_CONTENT_TYPE_TEXT_CSS,
			WEB_PAGE_STATIC_CSS, sizeof(WEB_PAGE_STATIC_CSS)-1);
	} else if (server.arg(String('r')) == F("js")) {
		server.send_P(200, TXT_CONTENT_TYPE_TEXT_JS,
			WEB_PAGE_STATIC_JS_CONFIG, sizeof(WEB_PAGE_STATIC_JS_CONFIG)-1);
	} else {
		_webserver_not_found();
	}
}

void SensorWebServer::_webserver_favicon() {
	// Keep the colored Robonomics PNG favicon (visible on light tab backgrounds); header logo stays device SVG.
	server.sendHeader(F("Cache-Control"), F("max-age=86400, public"));

	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
		ROBONOMICS_INFO_LOGO_PNG, ROBONOMICS_INFO_LOGO_PNG_SIZE);
}

void SensorWebServer::_webserver_restart() {
    if (!webserver_request_auth())
	{ return; }

	String page_content;
	page_content.reserve(512);

	start_html_page(page_content, FPSTR(INTL_RESTART_SENSOR));
	debug_outln_info(F("ws: reset ..."));

	if (server.method() == HTTP_GET) {
		page_content += FPSTR(WEB_RESET_CONTENT);
	} else {
		set_restart_reason(RESTART_REASON_USER);
		sensor_restart();
	}
	end_html_page(page_content);
}

void SensorWebServer::_webserver_removeConfig() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DELETE_CONFIG));
    bool is_HTTP_GET = server.method() == HTTP_GET;
	bool remove_all = false;
	if (server.hasArg("configType")) {
		const String server_arg(server.arg("configType"));
		remove_all = server_arg == "all";
	}
    webserver_removeConfig(page_content, is_HTTP_GET, remove_all);
    end_html_page(page_content);
    if (!is_HTTP_GET) {
        esp_restart();
    }
}

void SensorWebServer::_webserver_serial() {
    String s(Debug.popLines());

	server.send(s.length() ? 200 : 204, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), s);
}

void SensorWebServer::_webserver_wifi() {
    String page_content;
    webserver_wifi(wifiInfo, wifiInfoCount, page_content);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

void SensorWebServer::_webserver_debug_level() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEBUG_LEVEL));
    webserver_debug_level(server, page_content);
    end_html_page(page_content);;
}

void SensorWebServer::_webserver_values() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
    if (!webserver_request_auth())
		{ return; }
    RESERVE_STRING(page_content, XLARGE_STR);
    start_html_page(page_content, FPSTR(INTL_CURRENT_DATA));
	server.sendContent(page_content);
    webserver_values(sensors_data, page_content);
    end_html_page(page_content);
}

void SensorWebServer::_webserver_guest() {
    server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));
	// Enable Pagination (Chunked Transfer)
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	RESERVE_STRING(page_content, XLARGE_STR);

	start_html_page(page_content, FPSTR(INTL_CONFIGURATION));
    debug_outln_info(F("ws: guest ..."));

	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += FPSTR(WEB_CONFIG_SCRIPT);
	}

	if (server.method() == HTTP_GET) {
		webserver_guest_create_body_get_part1(page_content, wificonfig_loop, deviceStatus);
        // Paginate page after ~ 1500 Bytes
        server.sendContent(page_content);
	    page_content = emptyString;
        webserver_guest_create_body_get_part2(page_content, wificonfig_loop);
        server.sendContent(page_content);
	    page_content = emptyString;
	} else {
		webserver_config_send_body_post(server);
		server.sendContent(page_content);
		page_content = emptyString;
	}

	if (server.method() == HTTP_POST) {
			String page_content = F(
				"<body class='configuration'>"
				"<br>"
				"<div class='guest__connect-status guest__connect-status--initial'><h2 class='guest__connect-subtitle'>Connecting to WiFi...</h2>"
				"<div class='loader'></div></div>"
				"</body>"
				"</html>");

			server.sendContent(page_content);

			if (WiFi.status() != WL_CONNECTED) {
				if (cfg::wlannopwd) {
					debug_outln_info(F("No password"));
					WiFi.begin(cfg::wlanssid);
				} else {
					WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
				}
			}

			int counter = 0;
			while (WiFi.status() != WL_CONNECTED) {
				// Fail-fast: don't keep user stuck on "Connecting..." for too long.
				// 20 * 500ms = ~10s.
				if (counter > 20) {
					break;
				}
				delay(500);
				counter++;
			}

			if (WiFi.status() == WL_CONNECTED) {
				String address = WiFi.localIP().toString();
				debug_outln_info(F("Connected to WiFi network: "), cfg::wlanssid);
				page_content = "<script>document.querySelector('.guest__connect-status--initial').classList.add('hide');</script>";
				page_content += "<div class='guest__connected'><h2 class='guest__connect-title'>" INTL_GUEST_CONNECTED "</h2></div>\n";
				page_content += "<div class='guest__reboot guest__reboot--ip'>" INTL_GUEST_IP_ADDRESS " <span class='ip-address'>" + address + "</span> <button class='copy-btn' onclick='copyText()'></button></div>";
#ifdef ALTRUIST_INSIDE
				page_content += "<p class='guest__reboot' style='margin-top:10px;'>" INTL_GUEST_OPEN_IP_HINT "</p>";
#endif
				page_content += "<script>function copyText(){const e=document.querySelector('.ip-address').innerText;if(navigator.clipboard)navigator.clipboard.writeText(e).then((function(){alert('Copied to clipboard!')})).catch((function(e){alert('Failed to copy text')}));else{const o=document.createElement('textarea');o.value=e,document.body.appendChild(o),o.select(),document.execCommand('copy'),document.body.removeChild(o),alert('Copied to clipboard (fallback)')}}</script>";
				server.sendContent(page_content);

#ifdef ALTRUIST_INSIDE
				writeConfig();

				page_content = F(
					"<div style='margin:20px auto;max-width:480px;padding:20px;'>"
					"<p style='color:#444;font-size:15px;line-height:1.45;margin-bottom:18px;'>"
					INTL_SETUP_INSIGHT_MODE_HINT
					"</p>"
					"<form method='POST' action='/select_urban'>"
					"<label style='display:flex;align-items:flex-start;gap:12px;padding:14px 16px;"
					"border:1px solid #ddd;border-radius:8px;background:#fafafa;cursor:pointer;font-size:15px;line-height:1.35;'>"
					"<input type='checkbox' name='pair_with_urban' value='1' style='margin-top:3px;flex-shrink:0;'>"
					"<span>" INTL_SETUP_PAIR_WITH_URBAN "</span>"
					"</label>"
					"<button type='submit' class='submit-btn' style='margin-top:22px;width:100%;padding:14px;font-size:16px;'>"
					);
				page_content += F(INTL_SETUP_CONTINUE);
				page_content += F("</button></form></div>");
				server.sendContent(page_content);
				return;
#else
				// Urban flow: we already showed IP + hint above.
				// Don't send the same instruction twice.
				delay(5000);
#endif
			} else {
				page_content = F("<h2 class='guest__connect-subtitle error'>Connection Failed</h2>"
								"<p class='guest__reboot'>Failed to connect to: ");
				page_content += cfg::wlanssid;
				page_content += F("</p>");
#ifdef ALTRUIST_INSIDE
				page_content += F("<p class='guest__reboot'>Rebooting to WiFi setup… You can close this page and try again.</p>");
#endif
				server.sendContent(page_content);

#ifdef ALTRUIST_INSIDE
				// For Insight: reboot back into WiFi setup quickly so user can retry.
				if (writeConfig()) {
					set_restart_reason(RESTART_REASON_CONFIG);
					sensor_restart();
				}
				return;
#endif
			}

			if (writeConfig()) {
				set_restart_reason(RESTART_REASON_CONFIG);
				sensor_restart();
			}
		}
    // end_html_page(page_content);
}

void SensorWebServer::setWifiInfo(struct_wifiInfo* info, uint8_t count) {
    wifiInfo = info;
    wifiInfoCount = count;
}

#ifdef ALTRUIST_INSIDE
void SensorWebServer::_webserver_scan_urbans() {
	debug_outln_info(F("ws: scan_urbans ..."));
	String json = "[";
	int n = MDNS.queryService("altruist", "tcp");
	bool first = true;
	for (int i = 0; i < n; i++) {
		String device_type = "";
		if (MDNS.hasTxt(i, DEVICE_MODEL_MDNS_PROPERTY)) {
			device_type = MDNS.txt(i, DEVICE_MODEL_MDNS_PROPERTY);
		}
		if (device_type == DEVICE_MODEL_URBAN) {
			if (!first) json += ",";
			json += "{\"ip\":\"" + MDNS.address(i).toString() + "\",\"hostname\":\"" + MDNS.hostname(i) + "\"}";
			first = false;
		}
	}
	json += "]";
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json);
}

void SensorWebServer::_send_urban_pairing_form_html() {
	MDNS.begin(cfg::local_hostname);
	MDNS.addService("altruist", "tcp", 80);
	MDNS.addServiceTxt("altruist", "tcp", DEVICE_MODEL_MDNS_PROPERTY, DEVICE_MODEL);
	delay(1000);

	int nrOfServices = MDNS.queryService("altruist", "tcp");
	debug_outln_info(F("mDNS scan found services: "), String(nrOfServices));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_CONFIGURATION));

	page_content += F(
		"<div style='margin:20px auto;max-width:480px;padding:20px;'>"
		"<h3 style='margin-bottom:15px;text-align:center;'>" INTL_SELECT_URBAN_TITLE "</h3>"
		"<p style='color:#666;font-size:14px;margin-bottom:15px;text-align:center;'>"
		INTL_SELECT_URBAN_DESC "</p>"
		"<form method='POST' action='/select_urban'>");

	int urban_count = 0;
	for (int i = 0; i < nrOfServices; i++) {
		String device_type = "";
		if (MDNS.hasTxt(i, DEVICE_MODEL_MDNS_PROPERTY)) {
			device_type = MDNS.txt(i, DEVICE_MODEL_MDNS_PROPERTY);
		}
		if (device_type == DEVICE_MODEL_URBAN) {
			String ip = MDNS.address(i).toString();
			String hostname = MDNS.hostname(i);
			debug_outln_info(F("Found Urban: "), hostname + " (" + ip + ")");

			page_content += F("<div style='margin:8px 0;padding:12px;border:1px solid #ddd;border-radius:6px;background:#fafafa;'>"
				"<label style='cursor:pointer;display:flex;align-items:center;gap:10px;font-size:15px;'>"
				"<input type='radio' name='chosen_altruist_urban' value='");
			page_content += ip;
			page_content += "'";
			if (urban_count == 0) {
				page_content += F(" checked");
			}
			page_content += F("> <strong>");
			page_content += hostname;
			page_content += F("</strong>&nbsp;(");
			page_content += ip;
			page_content += F(")</label></div>");
			urban_count++;
		}
	}

	if (urban_count == 0) {
		page_content += F("<div style='padding:15px;background:#fff3cd;border:1px solid #ffc107;border-radius:6px;margin:10px 0;'>"
			"<p style='margin:0;color:#856404;'>" INTL_NO_URBANS_FOUND "</p></div>");
	}

	page_content += F("<div style='margin:8px 0;padding:12px;border:1px solid #ddd;border-radius:6px;background:#fafafa;'>"
		"<label style='cursor:pointer;display:flex;align-items:center;gap:10px;font-size:15px;'>"
		"<input type='radio' name='chosen_altruist_urban' value='__custom__'");
	if (urban_count == 0) {
		page_content += F(" checked");
	}
	page_content += F("> " INTL_USE_CUSTOM_IP "</label>"
		"<input type='text' name='custom_ip' placeholder='e.g. 192.168.1.100' "
		"style='margin-top:8px;padding:8px 12px;width:90%;max-width:220px;border:1px solid #ccc;border-radius:4px;font-size:14px;'/>"
		"</div>");

	page_content += F(
		"<button type='submit' class='submit-btn' style='margin-top:20px;width:100%;padding:14px;'>");
	page_content += FPSTR(INTL_SAVE_AND_RESTART);
	page_content += F("</button></form></div>");

	end_html_page(page_content);
}

void SensorWebServer::_webserver_select_urban() {
	debug_outln_info(F("ws: select_urban ..."));
	if (server.method() != HTTP_POST) {
		sendHttpRedirectGuest();
		return;
	}

	// Step 1: checkbox form from guest WiFi success (no chosen_altruist_urban field).
	if (!server.hasArg(F("chosen_altruist_urban"))) {
		const bool wantUrban = server.hasArg(F("pair_with_urban")) && server.arg(F("pair_with_urban")) == F("1");
		if (!wantUrban) {
			debug_outln_info(F("Insight standalone at setup (checkbox unchecked)"));
			cfg::standalone = true;
			cfg::use_custom_urban = false;
			cfg::chosen_altruist_urban[0] = '\0';
			cfg::custom_altruist_urban[0] = '\0';
		} else {
			_send_urban_pairing_form_html();
			return;
		}

		RESERVE_STRING(page_content, LARGE_STR);
		start_html_page(page_content, F(INTL_SETUP_COMPLETE));
		String setup_ip = WiFi.localIP().toString();
		page_content += F("<div style='text-align:center;padding:40px;'>"
			"<h2 style='color:#4CAF50;'>" INTL_SETTINGS_SAVED "</h2>"
			"<p>" INTL_GUEST_IP_ADDRESS " <strong><span class='ip-address'>");
		page_content += setup_ip;
		page_content += F("</span></strong> <button class='copy-btn' onclick='copyText()'></button></p>"
			"<p>" INTL_GUEST_OPEN_IP_HINT "</p>"
			"</div>"
			"<script>function copyText(){const e=document.querySelector('.ip-address').innerText;if(navigator.clipboard)navigator.clipboard.writeText(e).then((function(){alert('Copied to clipboard!')})).catch((function(e){alert('Failed to copy text')}));else{const o=document.createElement('textarea');o.value=e,document.body.appendChild(o),o.select(),document.execCommand('copy'),document.body.removeChild(o),alert('Copied to clipboard (fallback)')}}</script>");
		end_html_page(page_content);

		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
		return;
	}

	// Step 2: Urban IP / custom IP chosen.
	String chosen = server.arg("chosen_altruist_urban");

	if (chosen == "__skip__") {
		debug_outln_info(F("Urban selection skipped by user"));
		cfg::standalone = true;
		cfg::use_custom_urban = false;
		cfg::chosen_altruist_urban[0] = '\0';
		cfg::custom_altruist_urban[0] = '\0';
	} else if (chosen == "__custom__") {
		String custom_ip = server.arg("custom_ip");
		if (custom_ip.length() > 0) {
			strncpy(cfg::custom_altruist_urban, custom_ip.c_str(), LEN_CHOSEN_ALTRUIS_ADDRESS - 1);
			cfg::custom_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS - 1] = '\0';
			cfg::use_custom_urban = true;
			cfg::standalone = false;
			debug_outln_info(F("Custom Urban IP set: "), custom_ip);
		} else {
			cfg::standalone = true;
			cfg::use_custom_urban = false;
			cfg::chosen_altruist_urban[0] = '\0';
			cfg::custom_altruist_urban[0] = '\0';
			debug_outln_info(F("Custom Urban IP empty; standalone mode"));
		}
	} else if (chosen.length() > 0) {
		strncpy(cfg::chosen_altruist_urban, chosen.c_str(), LEN_CHOSEN_ALTRUIS_ADDRESS - 1);
		cfg::chosen_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS - 1] = '\0';
		cfg::use_custom_urban = false;
		cfg::standalone = false;
		debug_outln_info(F("Chosen Urban IP: "), chosen);
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, F(INTL_SETUP_COMPLETE));
	String setup_ip = WiFi.localIP().toString();
	page_content += F("<div style='text-align:center;padding:40px;'>"
		"<h2 style='color:#4CAF50;'>" INTL_SETTINGS_SAVED "</h2>"
		"<p>" INTL_GUEST_IP_ADDRESS " <strong><span class='ip-address'>");
	page_content += setup_ip;
	page_content += F("</span></strong> <button class='copy-btn' onclick='copyText()'></button></p>"
		"<p>" INTL_GUEST_OPEN_IP_HINT "</p>"
		"</div>"
		"<script>function copyText(){const e=document.querySelector('.ip-address').innerText;if(navigator.clipboard)navigator.clipboard.writeText(e).then((function(){alert('Copied to clipboard!')})).catch((function(e){alert('Failed to copy text')}));else{const o=document.createElement('textarea');o.value=e,document.body.appendChild(o),o.select(),document.execCommand('copy'),document.body.removeChild(o),alert('Copied to clipboard (fallback)')}}</script>");
	end_html_page(page_content);

	if (writeConfig()) {
		set_restart_reason(RESTART_REASON_CONFIG);
		sensor_restart();
	}
}
#endif

void SensorWebServer::_webserver_ota() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_OTA_UPDATE));

	if (server.method() == HTTP_POST) {
		if (server.hasArg("action") && server.arg("action") == F("switch_lang")) {
			String new_lang = server.arg("current_lang");
			new_lang.toUpperCase();
			if (new_lang == String(CURRENT_LANG)) {
				page_content += F("<div style='text-align:center;padding:30px;'>"
					"<h3 style='color:#FF9800;'>&#x26A0; ");
				page_content += FPSTR(INTL_OTA_LANG_SAME);
				page_content += F("</h3></div>");
			} else {
				strncpy(cfg::current_lang, new_lang.c_str(), sizeof(cfg::current_lang) - 1);
				cfg::current_lang[sizeof(cfg::current_lang) - 1] = '\0';
				writeConfig();
				deviceStatus.ota_update_requested = true;
				page_content += F("<div style='text-align:center;padding:30px;'>"
					"<h3 style='color:#4CAF50;'>&#x2713; ");
				page_content += FPSTR(INTL_OTA_LANG_REQUESTED);
				page_content += F("</h3></div>");
			}
			
		} else {
			deviceStatus.ota_update_requested = true;
			page_content += F("<div style='text-align:center;padding:30px;'>"
				"<h3 style='color:#4CAF50;'>&#x2713; ");
			page_content += FPSTR(INTL_OTA_CHECK_REQUESTED);
			page_content += F("</h3></div>");
		}
	} else {
		page_content += F("<div style='max-width:480px;margin:20px auto;padding:20px;'>");

		page_content += F("<table>");
		add_table_row_from_value(page_content, FPSTR(INTL_OTA_CURRENT_VERSION), String(SOFTWARE_VERSION_STR));
		add_table_row_from_value(page_content, FPSTR(INTL_LAST_OTA),
			delayToString(millis() - deviceStatus.last_update_attempt));
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);

		page_content += F("<form method='POST' action='/ota' style='text-align:center;margin-top:20px;'>"
			"<button type='submit' class='submit-btn'>");
		page_content += FPSTR(INTL_OTA_CHECK_UPDATE);
		page_content += F("</button></form>");

		// Language switch section
		page_content += F("<hr style='margin:30px 0;border:none;border-top:1px solid #ccc;'>");
		page_content += F("<h3 style='text-align:center;'>");
		page_content += FPSTR(INTL_OTA_SWITCH_LANG);
		page_content += F("</h3>");

		page_content += F("<table>");
		add_table_row_from_value(page_content, FPSTR(INTL_OTA_CURRENT_LANG), String(CURRENT_LANG));
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);

		String lang_select = F("<form method='POST' action='/ota' style='text-align:center;margin-top:10px;'>"
			"<input type='hidden' name='action' value='switch_lang'>"
			"<div class='form-group' style='margin-bottom:10px;'>"
			"<select name='current_lang' style='padding:8px;font-size:14px;'>"
			"<option value='EN'>English (EN)</option>"
			"<option value='RU'>Русский (RU)</option>"
			"</select></div>");
		lang_select.replace("'" + String(CURRENT_LANG) + "'>",
			"'" + String(CURRENT_LANG) + "' selected>");
		page_content += lang_select;
		page_content += F("<p style='color:#666;font-size:13px;margin:10px 0;'>");
		page_content += FPSTR(INTL_OTA_SWITCH_LANG_NOTE);
		page_content += F("</p>"
			"<button type='submit' class='submit-btn'>");
		page_content += FPSTR(INTL_OTA_SWITCH_LANG);
		page_content += F("</button></form>");

		page_content += F("</div>");
	}

	end_html_page(page_content);
}

void SensorWebServer::_webserver_config() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
	} else {
		if (!webserver_request_auth())
		{ return; }

		debug_outln_info(F("ws: config page ..."));

		server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
		server.sendHeader(F("Pragma"), F("no-cache"));
		server.sendHeader(F("Expires"), F("0"));
		// Enable Pagination (Chunked Transfer)
		server.setContentLength(CONTENT_LENGTH_UNKNOWN);

		RESERVE_STRING(page_content, XLARGE_STR);

		start_html_page(page_content, FPSTR(INTL_CONFIGURATION));
		if (wificonfig_loop) {  // scan for wlan ssids
			page_content += FPSTR(WEB_CONFIG_SCRIPT);
		}

		if (server.method() == HTTP_GET) {
			webserver_config_send_body_get(server, page_content, wificonfig_loop, sensors_data);
		} else {
			webserver_config_send_body_post(server);
			page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
			server.sendContent(page_content);
			page_content = emptyString;
		}
		end_html_page(page_content);

		if (server.method() == HTTP_POST) {

			if (writeConfig()) {
				set_restart_reason(RESTART_REASON_CONFIG);
				sensor_restart();
			}
		}
	}
}

void SensorWebServer::_webserver_root() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
    if (!webserver_request_auth())
		{ return; }

    RESERVE_STRING(page_content, XLARGE_STR);
    start_html_page(page_content, emptyString);
    debug_outln_info(F("ws: root ..."));
    webserver_root(page_content, robonomics_address);
    end_html_page_root(page_content);
}

bool SensorWebServer::webserver_request_auth() {
	if (cfg::www_basicauth_enabled && ! wificonfig_loop) {
		debug_outln_info(F("validate request auth..."));
		if (!server.authenticate(www_username.c_str(), www_password.c_str())) {
			server.requestAuthentication(BASIC_AUTH, "Sensor Login", F("Authentication failed"));
			return false;
		}
	}
	return true;
}

void SensorWebServer::sendHttpRedirectGuest() {
	// Never hard-code 192.168.4.1 in STA-only mode: after a home-WiFi drop the device is not that AP,
	// and browsers following this redirect appear "stuck" / webserver dead while HTTP is still up.
	const IPAddress ap_ip = WiFi.softAPIP();
	const IPAddress sta_ip = WiFi.localIP();
	String loc = F("http://");
	if (wificonfig_loop && ap_ip[0] != 0) {
		loc += ap_ip.toString();
	} else if (sta_ip[0] != 0) {
		loc += sta_ip.toString();
	} else {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN),
		             F("WiFi not ready yet; try again in a few seconds."));
		return;
	}
	loc += F("/guest");
	server.sendHeader(F("Location"), loc);
	server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
}

void SensorWebServer::sendHttpRedirectConnected(String &address) {
	String redirect = F("http://");
	redirect += address;
	debug_outln_info(F("Redirecting to: "), redirect);
	server.sendHeader(F("Location"), redirect);
	server.sendHeader(F("Cache-Control"), F("no-cache"));
	server.sendHeader(F("Connection"), F("close"));
	server.send(303, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), "<html><body>Redirecting...</body></html>");

}

void SensorWebServer::start_html_page(String& page_content, const String& title) {
	RESERVE_STRING(s, LARGE_STR);
	s = FPSTR(WEB_PAGE_HEADER);
	s.replace("{t}", title);
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), s);

	if(title.indexOf(INTL_CONFIGURATION) != -1) {
		server.sendContent_P(WEB_PAGE_HEADER_CONFIG_HEAD);
	} else {
		server.sendContent_P(WEB_PAGE_HEADER_HEAD);
	}

	if (title.indexOf(INTL_DEBUG_LEVEL) != -1) {
		s = FPSTR(WEB_PAGE_DEBUG_HEADER_BODY);
	} else if (title.indexOf(INTL_CONFIGURATION) != -1) {
		s = FPSTR(WEB_PAGE_CONFIG_HEADER_BODY);
	} else {
		s = FPSTR(WEB_PAGE_HEADER_BODY);
	}
	s.replace("{addr}", robonomics_address);
	s.replace("{t}", title);
	if (title != " ") {
		s.replace("{n}", F("&raquo;"));
	} else {
		s.replace("{n}", emptyString);
	}
	s.replace("{id}", esp_chipid);
	s.replace("{mac}", WiFi.macAddress());
	page_content += s;
}

void SensorWebServer::end_html_page(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
	}
	server.sendContent_P(WEB_PAGE_FOOTER);
}

void SensorWebServer::end_html_page_root(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
	}
	server.sendContent_P(WEB_PAGE_ROOT_FOOTER);
}