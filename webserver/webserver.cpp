#include "webserver.h"
#include "pages/pages.h"
#include "html-content.h"
#include <WiFi.h>
#if defined(ALTRUIST_URBAN_C3_LITE)
#include "script-js-lite.h"
#else
#include "script-js.h"
#endif
#include "utils.h"
#include "../config_manager/config_helpers.h"
#include "../config_manager/device_backup.h"
#include "../apis/rws_group.h"
#include "../defines.h"
#include "../utils.h"
#include "../wifi_manager.h"
#include "../OTA_Update.h"
#include <Robonomics.h>
#include "web-header-logo-select.h"
#include "favicon.h"
#if !defined(ALTRUIST_URBAN_C3_LITE)
#include "nav-icons.h"
#endif

extern Robonomics robonomics;

#ifdef ALTRUIST_INSIGHT
#include <ESPmDNS.h>
#include "display/display_manager.h"
extern DisplayManager displayManager;

static unsigned long s_insight_guest_finish_deadline_ms = 0;

static void insightGuestApplyStandaloneAndRestart() {
	cfg::standalone = true;
	cfg::use_custom_urban = false;
	cfg::chosen_altruist_urban[0] = '\0';
	cfg::custom_altruist_urban[0] = '\0';
	cfgApplyStandaloneModeEnabled();
	if (writeConfig()) {
		set_restart_reason(RESTART_REASON_CONFIG);
		sensor_restart();
	}
}

void insightGuestMarkFinishPending(void) {
	s_insight_guest_finish_deadline_ms = millis() + INSIGHT_GUEST_AUTO_FINISH_MS;
}

void insightGuestClearFinishPending(void) {
	s_insight_guest_finish_deadline_ms = 0;
}

void insightGuestProcessPendingFinish(void) {
	if (s_insight_guest_finish_deadline_ms == 0) {
		return;
	}
	if ((long)(millis() - s_insight_guest_finish_deadline_ms) < 0) {
		return;
	}
	if (!wifiGuestPortalStaReady()) {
		return;
	}
	s_insight_guest_finish_deadline_ms = 0;
	debug_outln_info(F("Insight guest setup: auto-finish (standalone)"));
	insightGuestApplyStandaloneAndRestart();
}
#endif

static SemaphoreHandle_t s_webserver_mutex = nullptr;

static bool webserverLock(uint32_t timeout_ms = 250) {
	if (!s_webserver_mutex) {
		s_webserver_mutex = xSemaphoreCreateMutex();
	}
	if (!s_webserver_mutex) {
		return false;
	}
	return xSemaphoreTake(s_webserver_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

static void webserverUnlock() {
	if (s_webserver_mutex) {
		xSemaphoreGive(s_webserver_mutex);
	}
}

void SensorWebServer::handleClient() {
	// Guard against races between loop() and the captive portal worker.
	// Retry briefly: a single 250ms miss drops phone requests (HTML + CSS + logo in parallel).
	for (uint8_t attempt = 0; attempt < 4; ++attempt) {
		if (!webserverLock(attempt == 0 ? 250 : 100)) {
			yield();
			continue;
		}
		for (uint8_t i = 0; i < 4; ++i) {
			server.handleClient();
			markMainLoopAlive();
			yield();
		}
		webserverUnlock();
		return;
	}
}


void SensorWebServer::setup() {
    www_username = cfg::www_username;
    www_password = cfg::www_password;
    uint64_t chipid_num;
	esp_chipid = get_chipid();

	server.on("/guest", HTTP_GET, std::bind(&SensorWebServer::_webserver_guest, this));
	server.on("/guest", HTTP_POST, std::bind(&SensorWebServer::_webserver_guest, this));
	server.on(F("/guest-restore"), HTTP_POST, std::bind(&SensorWebServer::_webserver_restore_backup_post, this),
	         std::bind(&SensorWebServer::_webserver_restore_backup_upload, this));
	server.on("/", std::bind(&SensorWebServer::_webserver_hub_local, this)); // x
	server.on(F("/local"), std::bind(&SensorWebServer::_webserver_hub_local, this));
	server.on(F("/social"), std::bind(&SensorWebServer::_webserver_hub_social, this));
	server.on(F("/custom"), std::bind(&SensorWebServer::_webserver_hub_custom, this));
	server.on(F("/advanced"), std::bind(&SensorWebServer::_webserver_hub_advanced, this));
	server.on(F("/warnings"), std::bind(&SensorWebServer::_webserver_hub_warnings_redirect, this));
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
	server.on(F("/favicon-dark.ico"), std::bind(&SensorWebServer::_webserver_favicon_dark, this)); // x
	server.on(F("/device-info.json"), std::bind(&SensorWebServer::_webserver_device_info_json, this));
	server.on(F("/owner-access.json"), std::bind(&SensorWebServer::_webserver_owner_access_json, this));
	server.on(F("/backup.json"), std::bind(&SensorWebServer::_webserver_backup_json, this));
	server.on(F("/restore-backup"), HTTP_POST, std::bind(&SensorWebServer::_webserver_restore_backup_post, this),
	         std::bind(&SensorWebServer::_webserver_restore_backup_upload, this));
	server.on(F(STATIC_PREFIX), std::bind(&SensorWebServer::_webserver_static, this)); // x
	server.on(F("/ota"), std::bind(&SensorWebServer::_webserver_ota, this));
	server.on(F("/ota-check.json"), HTTP_GET, std::bind(&SensorWebServer::_webserver_ota_check_json, this));
	server.on(F("/ota-install.json"), HTTP_POST, std::bind(&SensorWebServer::_webserver_ota_install_json, this));
	server.on(F("/ota-progress.json"), HTTP_GET, std::bind(&SensorWebServer::_webserver_ota_progress_json, this));
	server.on(F("/finish_setup"), std::bind(&SensorWebServer::_webserver_finish_setup, this));
	server.on(F("/group"), std::bind(&SensorWebServer::_webserver_group, this));
#ifdef ALTRUIST_INSIGHT
	server.on(F("/screen"), std::bind(&SensorWebServer::_webserver_screen, this));
	server.on(F("/select_urban"), std::bind(&SensorWebServer::_webserver_select_urban, this));
	server.on(F("/scan_urbans"), std::bind(&SensorWebServer::_webserver_scan_urbans, this));
	server.on(F("/guest_setup_ack"), [this]() {
		insightGuestClearFinishPending();
		server.send(204, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), emptyString);
	});
#endif
	server.onNotFound(std::bind(&SensorWebServer::_webserver_not_found, this)); // x

	String listen_ip = WiFi.localIP().toString();
	if (listen_ip == F("0.0.0.0")) {
		const IPAddress ap_ip = WiFi.softAPIP();
		if (ap_ip[0] != 0) {
			listen_ip = ap_ip.toString();
		}
	}
	debug_outln_info(F("Starting Webserver... "), listen_ip);
	server.begin();
}

void SensorWebServer::notifyStaIpRestored() {
#if defined(ESP32)
	// WebServer::begin() calls close() internally, but after a netif/IP change an explicit stop + short yield
	// avoids a stuck listen socket so phones/browsers can reach the device again on the LAN IP.
	debug_outln_info(F("Webserver: rebind after STA IP "), WiFi.localIP().toString());
	if (!webserverLock(2000)) {
		debug_outln_error(F("[WiFi] Webserver rebind skipped: mutex busy"));
		return;
	}
	server.stop();
	yield();
	delay(30);
	server.begin();
	webserverUnlock();
#endif
}

void SensorWebServer::_webserver_status() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEVICE_STATUS), false, "status");

	debug_outln_info(F("ws: status ..."));
    webserver_status_part1(page_content, deviceStatus, server);
    web_page_flush_chunk(page_content, &server);
    webserver_status_part2(page_content, deviceStatus, server);

	page_content += F("</div></div>");
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_data_json() {
	String json_content;
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(300))) {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_JSON), F("{\"error\":\"busy\"}"));
		return;
	}
	webserver_data_json(sensors_data, esp_chipid, json_content);
	xSemaphoreGive(mutex);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json_content);
}

void SensorWebServer::_webserver_not_found() {
	debug_outln_info(F("ws: not found: "), server.uri());
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
	const String resource = server.arg(String('r'));
#if !defined(ALTRUIST_URBAN_C3_LITE)
	if (resource == F("nav-local")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_LOCAL_PNG, WEB_NAV_ICON_LOCAL_PNG_SIZE);
		return;
	}
	if (resource == F("nav-map")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_MAP_PNG, WEB_NAV_ICON_MAP_PNG_SIZE);
		return;
	}
	if (resource == F("nav-custom")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_CUSTOM_PNG, WEB_NAV_ICON_CUSTOM_PNG_SIZE);
		return;
	}
	if (resource == F("nav-system")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_SYSTEM_PNG, WEB_NAV_ICON_SYSTEM_PNG_SIZE);
		return;
	}
#endif
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
	server.sendHeader(F("Cache-Control"), F("max-age=86400, public"));
	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_FAVICON_PNG, WEB_FAVICON_PNG_SIZE);
}

void SensorWebServer::_webserver_favicon_dark() {
	server.sendHeader(F("Cache-Control"), F("max-age=86400, public"));
	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_FAVICON_DARK_PNG, WEB_FAVICON_DARK_PNG_SIZE);
}

void SensorWebServer::_webserver_device_info_json() {
	if (!webserver_request_auth()) {
		return;
	}

	DynamicJsonDocument doc(1024);
	doc["format"] = "altruist-device1";
	doc["hostname"] = buildDeviceAccessHost();
	if (WiFi.status() == WL_CONNECTED) {
		doc["ip"] = WiFi.localIP().toString();
	}
	if (robonomics_address.length() > 0 && strcasecmp(robonomics_address.c_str(), "Not Set") != 0) {
		doc["sensor"] = robonomics_address;
	}

	String body;
	serializeJson(doc, body);
	server.sendHeader(F("Content-Disposition"), F("attachment; filename=\"altruist-device-info.json\""));
	server.sendHeader(F("Cache-Control"), F("no-store"));
	// octet-stream so browsers download instead of opening JSON in the tab
	server.send(200, F("application/octet-stream"), body);
}

void SensorWebServer::_webserver_owner_access_json() {
	if (!webserver_request_auth()) {
		return;
	}

	const char *sk = cfg::private_key;
	if (!sk || strcasecmp(sk, "Not Set") == 0 || strlen(sk) < 64) {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Device private key unavailable"));
		return;
	}
	if (robonomics_address.length() == 0 || strcasecmp(robonomics_address.c_str(), "Not Set") == 0) {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Device address unavailable"));
		return;
	}

	DynamicJsonDocument doc(768);
	doc["format"] = "altruist-owner1";
	doc["type"] = "ed25519";
	doc["address"] = robonomics_address;
	doc["seed"] = sk;
	doc["sensor"] = robonomics_address;
	doc["hint"] = "Import this file on sensors.map Login to decrypt self-owner encrypted metrics";

	String body;
	serializeJson(doc, body);
	server.sendHeader(F("Content-Disposition"), F("attachment; filename=\"altruist-owner-access.json\""));
	server.sendHeader(F("Cache-Control"), F("no-store"));
	server.send(200, F("application/octet-stream"), body);
}

void SensorWebServer::_webserver_backup_json() {
	if (!webserver_request_auth()) {
		return;
	}

	const bool include_owner =
	    cfg::private_key[0] != '\0' && strcasecmp(cfg::private_key, "Not Set") != 0 && strlen(cfg::private_key) >= 64 &&
	    robonomics_address.length() > 0 && strcasecmp(robonomics_address.c_str(), "Not Set") != 0;

	String body;
	if (!buildDeviceBackupJson(body, robonomics_address, buildDeviceAccessHost(), include_owner)) {
		server.send(507, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Backup too large; contact support"));
		return;
	}

	server.sendHeader(F("Content-Disposition"), F("attachment; filename=\"altruist-backup.json\""));
	server.sendHeader(F("Cache-Control"), F("no-store"));
	server.send(200, F("application/octet-stream"), body);
}

void SensorWebServer::_webserver_restore_backup_upload() {
	HTTPUpload& upload = server.upload();
	const char* const field_name = upload.name.c_str();
	if (field_name == nullptr) {
		return;
	}
	if (upload.status == UPLOAD_FILE_START) {
		if (strcmp(field_name, "backup") != 0) {
			return;
		}
		debug_outln_info(F("ws: restore-backup upload start: "), upload.filename);
		backup_upload_body = "";
		backup_upload_size = 0;
		backup_upload_overflow = false;
	} else if (upload.status == UPLOAD_FILE_WRITE) {
		if (strcmp(field_name, "backup") != 0 || backup_upload_overflow) {
			return;
		}
		if (backup_upload_size + upload.currentSize > JSON_BUFFER_SIZE) {
			debug_outln_error(F("ws: restore-backup upload too large"));
			backup_upload_overflow = true;
			return;
		}
		backup_upload_body.concat(reinterpret_cast<const char*>(upload.buf), upload.currentSize);
		backup_upload_size += upload.currentSize;
	} else if (upload.status == UPLOAD_FILE_END) {
		if (strcmp(field_name, "backup") != 0) {
			return;
		}
		debug_outln_info(F("ws: restore-backup upload end, bytes: "), String(backup_upload_size));
	} else if (upload.status == UPLOAD_FILE_ABORTED) {
		debug_outln_error(F("ws: restore-backup upload aborted"));
		backup_upload_body = "";
		backup_upload_size = 0;
		backup_upload_overflow = true;
	}
}

void SensorWebServer::_webserver_restore_backup_post() {
	debug_outln_info(F("ws: restore-backup POST ..."));
	if (!webserver_request_auth()) {
		backup_upload_body = "";
		backup_upload_size = 0;
		backup_upload_overflow = false;
		return;
	}

	const bool guest_setup = wificonfig_loop;
	RESERVE_STRING(page_content, LARGE_STR);

	if (guest_setup) {
		start_html_page(page_content, FPSTR(INTL_DEVICE_BACKUP_TITLE), true);
		page_content += F("<div class='guest-page'><div class='guest-card'>");
	} else {
		start_html_page(page_content, FPSTR(INTL_DEVICE_BACKUP_TITLE), false, "settings");
		append_app_page_body_start(page_content, FPSTR(INTL_DEVICE_BACKUP_RESTORE_HINT));
	}

	bool ok = false;
	if (backup_upload_overflow || backup_upload_size == 0 || backup_upload_size > JSON_BUFFER_SIZE) {
		debug_outln_error(F("ws: restore-backup empty/overflow upload"));
		ok = false;
	} else {
		DynamicJsonDocument doc(JSON_BUFFER_SIZE);
		const DeserializationError err = deserializeJson(doc, backup_upload_body);
		if (err) {
			debug_outln_error(F("ws: restore-backup JSON parse failed"));
			debug_outln_info(F("ws: restore-backup parse: "), String(err.c_str()));
		} else {
			const DeviceBackupRestoreResult result = restoreDeviceBackupFromJson(doc);
			ok = (result == DeviceBackupRestoreResult::Ok);
			if (!ok) {
				debug_outln_error(F("ws: restore-backup apply failed"));
				debug_outln_info(F("ws: restore-backup result: "), String(static_cast<uint8_t>(result)));
			}
		}
	}

	backup_upload_body = "";
	backup_upload_size = 0;
	backup_upload_overflow = false;

	if (ok) {
		page_content += F("<div class='ui-notice ui-notice--ok'><strong>");
		page_content += FPSTR(INTL_DEVICE_BACKUP_RESTORE_OK);
		page_content += F("</strong></div>");
		if (guest_setup) {
			page_content += F("<p class='form-hint'>");
			page_content += FPSTR(INTL_DEVICE_BACKUP_RESTORE_OK);
			page_content += F("</p>");
		}
	} else {
		page_content += F("<div class='ui-notice ui-notice--err'><strong>");
		page_content += FPSTR(INTL_DEVICE_BACKUP_RESTORE_FAILED);
		page_content += F("</strong></div>");
		if (guest_setup) {
			page_content += F("<p class='form-hint'><a href='/guest'>" INTL_BACK_TO_HOME "</a></p>");
		}
	}

	if (guest_setup) {
		page_content += F("</div></div>");
		end_html_page_guest(page_content);
	} else {
		append_app_page_body_end(page_content);
		end_html_page_app(page_content);
	}

	if (ok) {
		debug_outln_info(F("ws: restore-backup OK, restarting"));
		Serial.flush();
		delay(400);
		set_restart_reason(RESTART_REASON_USER);
		sensor_restart();
	}
}

void SensorWebServer::_webserver_restart() {
    if (!webserver_request_auth())
	{ return; }

	String page_content;
	page_content.reserve(512);

	start_html_page(page_content, FPSTR(INTL_RESTART_SENSOR), false, "settings");
	debug_outln_info(F("ws: reset ..."));

	append_app_page_body_start(page_content, F(INTL_PAGE_RESTART_INTRO));

	if (server.method() == HTTP_GET) {
		page_content += F("<section class='app-panel app-panel--confirm'>");
		page_content += FPSTR(WEB_RESET_CONTENT);
		page_content += F("</section>");
	} else {
		set_restart_reason(RESTART_REASON_USER);
		sensor_restart();
	}
	append_app_page_body_end(page_content);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_removeConfig() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, String(F(INTL_CONFIGURATION_DELETE)), false, "settings");
	append_app_page_body_start(page_content, F(INTL_PAGE_DELETE_CONFIG_INTRO));
    bool is_HTTP_GET = server.method() == HTTP_GET;
	bool remove_all = false;
	if (server.hasArg("configType")) {
		const String server_arg(server.arg("configType"));
		remove_all = server_arg == "all";
	}
    webserver_removeConfig(page_content, is_HTTP_GET, remove_all);
	append_app_page_body_end(page_content);
    end_html_page_app(page_content);
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
#if defined(ESP32) || defined(ESP8266)
	if (wificonfig_loop) {
		uint8_t count = 0;
		struct_wifiInfo* cached = wifiPortalScanCache(&count);
		webserver_wifi(cached, count, page_content);
		server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
		return;
	}
#endif
	if (wifiInfo == nullptr || wifiInfoCount == 0) {
		webserver_wifi(nullptr, 0, page_content);
	} else {
		webserver_wifi(wifiInfo, wifiInfoCount, page_content);
	}
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

void SensorWebServer::_webserver_debug_level() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEBUG_LEVEL), false, "settings");
	append_app_page_body_start(page_content, F(INTL_PAGE_DEBUG_INTRO));
    webserver_debug_level(server, page_content);
	append_app_page_body_end(page_content);
    end_html_page_app(page_content);
}

void SensorWebServer::_webserver_group() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_GROUP_MENU), false, "settings");

	const String self_ss58 = String(robonomics.getSs58Address());
	setRobonomicsAddress(self_ss58);
	rwsSyncGroupModeFromOwner(self_ss58);

	RwsGroupApplyResult save_result = RwsGroupApply_None;
	if (server.method() == HTTP_POST) {
		save_result = webserver_group_post(server, self_ss58);
	}

	webserver_group_page(page_content, self_ss58, &robonomics, save_result);
	end_html_page_app(page_content);
}

#ifdef ALTRUIST_INSIGHT
void SensorWebServer::_webserver_screen() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_SCREEN_MENU), false, "settings");

	ScreenSaveResult save_result = ScreenSave_None;
	if (server.method() == HTTP_POST) {
		save_result = webserver_screen_post(server);
	}

	webserver_screen_page(page_content, save_result);
	end_html_page_app(page_content);
}
#endif

void SensorWebServer::_webserver_values() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
    if (!webserver_request_auth())
		{ return; }
    RESERVE_STRING(page_content, XLARGE_STR);
    start_html_page(page_content, FPSTR(INTL_CURRENT_DATA), false, "values");
	DynamicJsonDocument values_snapshot(sensors_data.capacity());
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		page_content = F("<p class='data-busy-msg'>");
		page_content += FPSTR(INTL_DATA_BUSY);
		page_content += F("</p>");
	} else {
		values_snapshot.set(sensors_data.as<JsonVariantConst>());
		xSemaphoreGive(mutex);
		if (values_snapshot.overflowed()) {
			page_content = F("<p class='data-busy-msg'>");
			page_content += FPSTR(INTL_DATA_BUSY);
			page_content += F("</p>");
		} else {
			webserver_values(values_snapshot, page_content, server);
		}
	}
    end_html_page_app(page_content);
}

void SensorWebServer::_webserver_guest() {
    server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
    server.sendHeader(F("Expires"), F("0"));

	if (server.method() == HTTP_POST) {
		backup_upload_body = "";
		backup_upload_size = 0;
		backup_upload_overflow = false;
		debug_outln_info(F("ws: guest POST ..."));
		webserver_config_send_body_post(server);

		stream_html_page_head(FPSTR(INTL_CONFIGURATION), true);
		server.sendContent_P(WEB_GUEST_CONNECT_STATUS);
		yield();

#if defined(ESP32) || defined(ESP8266)
		wifiGuestPortalPrepareStaJoin();
		wifiApplyStaHostname();
#endif
		if (cfg::wlannopwd) {
			debug_outln_info(F("No password"));
			WiFi.begin(cfg::wlanssid);
		} else {
			WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
		}

		int counter = 0;
		while (!wifiGuestPortalStaReady()) {
			if (counter >= 80) {
				break;
			}
			yield();
			delay(counter < 30 ? 100 : 250);
			counter++;
		}

		RESERVE_STRING(page_content, XLARGE_STR);

		if (wifiGuestPortalStaReady()) {
			String address = WiFi.localIP().toString();
			debug_outln_info(F("Connected to WiFi network: "), cfg::wlanssid);
			debug_outln_info(F("STA IP: "), address);
			page_content = F("<script>var gc=document.getElementById('guest-connecting');if(gc)gc.classList.add('hide');</script>");
#ifdef ALTRUIST_INSIDE
			const unsigned insightAutoSec = (unsigned)(INSIGHT_GUEST_AUTO_FINISH_MS / 1000UL);
			page_content += F("<div class='guest-page'><div class='guest-card guest__setup-finish'>");
			page_content += F("<div class='guest__setup-header'>"
				"<span class='guest__step-label'>" INTL_GUEST_SETUP_STEP_2_LABEL "</span>"
				"<h2 class='guest__step-title'>" INTL_GUEST_WIFI_STEP_TITLE "</h2>"
				"</div>");
			append_guest_device_access(page_content, address, robonomics_address);
			page_content += F("<p class='form-hint guest-hint'>" INTL_GUEST_INSIGHT_FINISH_HINT "</p>");
			page_content += F("<p class='form-hint'>" INTL_GUEST_KEEP_OPEN_HINT "</p>");
			page_content += F("<p class='form-hint' id='insight-auto-finish-hint'>"
				INTL_GUEST_INSIGHT_AUTO_FINISH_HINT " <strong id='insight-auto-sec'>");
			page_content += String(insightAutoSec);
			page_content += F("</strong> " INTL_GUEST_INSIGHT_AUTO_FINISH_SUFFIX "</p>");

			if (!writeConfig()) {
				page_content += F("<p class='guest__reboot error'>Failed to save configuration.</p></div></div>");
				end_html_page_guest(page_content);
				return;
			}
			insightGuestMarkFinishPending();

			page_content += F("<p class='form-hint'>" INTL_SETUP_INSIGHT_MODE_HINT "</p>");
			page_content += F("<form id='insight-finish-form' class='guest-wizard-form' method='POST' action='/select_urban'>"
				"<label class='guest-option'>"
				"<input type='checkbox' name='pair_with_urban' value='1'>"
				"<span>" INTL_SETUP_PAIR_WITH_URBAN "</span>"
				"</label>"
				"<button type='submit' class='submit-btn guest__setup-finish-btn'>");
			page_content += F(INTL_SETUP_CONTINUE);
			page_content += F("</button></form></div></div>");
			page_content += F("<script>(function(){var s=");
			page_content += String(insightAutoSec);
			page_content += F(",el=document.getElementById('insight-auto-sec'),form=document.getElementById('insight-finish-form');"
				"function tick(){if(s>0){if(el)el.textContent=String(s);s--;}else if(form)form.submit();}"
				"tick();setInterval(tick,1000);})();</script>");
			page_content += FPSTR(WEB_GUEST_WIZARD_SUBMIT_JS);
			end_html_page_guest(page_content);
			return;
#else
			page_content += F("<div class='guest-page'><div class='guest-card'>");
			page_content += F("<div class='guest__connected'><h2 class='guest__connect-title'>" INTL_GUEST_CONNECTED "</h2></div>");
			append_guest_device_access(page_content, address, robonomics_address);
			page_content += F("<p class='form-hint'>" INTL_GUEST_OPEN_IP_HINT "</p>");

			if (!writeConfig()) {
				page_content += F("<p class='guest__reboot error'>Failed to save configuration.</p></div></div>");
				end_html_page_guest(page_content);
				return;
			}
			append_guest_success_restart_ui(page_content);
			page_content += F("</div></div>");
			end_html_page_guest(page_content);
			guestSuccessMarkRestartPending();
			return;
#endif
		}

		page_content = F("<script>var gc=document.getElementById('guest-connecting');if(gc)gc.classList.add('hide');</script>");
		page_content += F("<div class='guest-page'><div class='guest-card guest-card--connect'>"
			"<div class='guest__connect-status'>"
			"<h2 class='guest__connect-subtitle error'>" INTL_GUEST_CONNECT_FAILED "</h2>"
			"<p class='form-hint'>");
		page_content += cfg::wlanssid;
		page_content += F("</p>");
#ifdef ALTRUIST_INSIDE
		page_content += F("<p class='form-hint'>" INTL_GUEST_CONNECT_FAILED_INSIGHT "</p>"
			"<p class='form-hint'><a href='/guest'>" INTL_BACK_TO_HOME "</a></p>");
#else
		page_content += F("<p class='form-hint'>" INTL_GUEST_CONNECT_FAILED_HINT "</p>");
#endif
		page_content += F("</div></div></div>");
		end_html_page_guest(page_content);
		// Keep credentials in SPIFFS but stay in the captive portal so the user can retry.
		writeConfig();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);

	start_html_page(page_content, FPSTR(INTL_CONFIGURATION), true);
    debug_outln_info(F("ws: guest GET ..."));

	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += FPSTR(WEB_CONFIG_SCRIPT);
	}

	webserver_guest_create_body_get_part1(page_content, wificonfig_loop, deviceStatus);
	web_page_flush_chunk(page_content, &server);
	webserver_guest_create_body_get_part2(page_content, wificonfig_loop);
	web_page_flush_chunk(page_content, &server);
	end_html_page_guest(page_content);
}

void SensorWebServer::setWifiInfo(struct_wifiInfo* info, uint8_t count) {
    wifiInfo = info;
    wifiInfoCount = count;
}

#ifdef ALTRUIST_INSIGHT
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
	delay(250);
	yield();

	int nrOfServices = MDNS.queryService("altruist", "tcp");
	debug_outln_info(F("mDNS scan found services: "), String(nrOfServices));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_CONFIGURATION), true);

	page_content += F(
		"<div style='margin:20px auto;max-width:480px;padding:20px;'>"
		"<h3 style='margin-bottom:15px;text-align:center;'>" INTL_SELECT_URBAN_TITLE "</h3>"
		"<p style='color:#666;font-size:14px;margin-bottom:15px;text-align:center;'>"
		INTL_SELECT_URBAN_DESC "</p>"
		"<form class='guest-wizard-form' method='POST' action='/select_urban'>");

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
	page_content += FPSTR(WEB_GUEST_WIZARD_SUBMIT_JS);

	end_html_page_guest(page_content);
}

void SensorWebServer::_webserver_select_urban() {
	debug_outln_info(F("ws: select_urban ..."));
	if (server.method() != HTTP_POST) {
		sendHttpRedirectGuest();
		return;
	}
#ifdef ALTRUIST_INSIGHT
	insightGuestClearFinishPending();
#endif

	// Step 1: checkbox form from guest WiFi success (no chosen_altruist_urban field).
	if (!server.hasArg(F("chosen_altruist_urban"))) {
		const bool wantUrban = server.hasArg(F("pair_with_urban")) && server.arg(F("pair_with_urban")) == F("1");
		if (!wantUrban) {
			debug_outln_info(F("Insight standalone at setup (checkbox unchecked)"));
			cfg::standalone = true;
			cfg::use_custom_urban = false;
			cfg::chosen_altruist_urban[0] = '\0';
			cfg::custom_altruist_urban[0] = '\0';
			cfgApplyStandaloneModeEnabled();
		} else {
			_send_urban_pairing_form_html();
			return;
		}

		RESERVE_STRING(page_content, LARGE_STR);
		start_html_page(page_content, F(INTL_SETUP_COMPLETE), true);
		String setup_ip = WiFi.localIP().toString();
		page_content += F("<div class='guest-page'><div class='guest-card'>"
			"<div class='guest__connected'><h2 class='guest__connect-title'>" INTL_SETTINGS_SAVED "</h2></div>");
		append_guest_device_access(page_content, setup_ip, robonomics_address);
		if (!writeConfig()) {
			page_content += F("<p class='guest__reboot error'>Failed to save configuration.</p></div></div>");
			end_html_page_guest(page_content);
			return;
		}
		append_guest_success_restart_ui(page_content);
		page_content += F("</div></div>");
		end_html_page_guest(page_content);
		guestSuccessMarkRestartPending();
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
		cfgApplyStandaloneModeEnabled();
	} else if (chosen == "__custom__") {
		String custom_ip = server.arg("custom_ip");
		if (custom_ip.length() > 0) {
			strncpy(cfg::custom_altruist_urban, custom_ip.c_str(), LEN_CHOSEN_ALTRUIS_ADDRESS - 1);
			cfg::custom_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS - 1] = '\0';
			cfg::use_custom_urban = true;
			cfg::standalone = false;
			cfgOnStandaloneModeDisabled();
			debug_outln_info(F("Custom Urban IP set: "), custom_ip);
		} else {
			cfg::standalone = true;
			cfg::use_custom_urban = false;
			cfg::chosen_altruist_urban[0] = '\0';
			cfg::custom_altruist_urban[0] = '\0';
			debug_outln_info(F("Custom Urban IP empty; standalone mode"));
			cfgApplyStandaloneModeEnabled();
		}
	} else if (chosen.length() > 0) {
		strncpy(cfg::chosen_altruist_urban, chosen.c_str(), LEN_CHOSEN_ALTRUIS_ADDRESS - 1);
		cfg::chosen_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS - 1] = '\0';
		cfg::use_custom_urban = false;
		cfg::standalone = false;
		cfgOnStandaloneModeDisabled();
		debug_outln_info(F("Chosen Urban IP: "), chosen);
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, F(INTL_SETUP_COMPLETE), true);
	String setup_ip = WiFi.localIP().toString();
	page_content += F("<div class='guest-page'><div class='guest-card'>"
		"<div class='guest__connected'><h2 class='guest__connect-title'>" INTL_SETTINGS_SAVED "</h2></div>");
	append_guest_device_access(page_content, setup_ip, robonomics_address);

	if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		clearUrbanPairingTelemetry(sensors_data);
		xSemaphoreGive(mutex);
	}
	displayManager.clearUrbanCache();

	if (!writeConfig()) {
		page_content += F("<p class='guest__reboot error'>Failed to save configuration.</p></div></div>");
		end_html_page_guest(page_content);
		return;
	}
	append_guest_success_restart_ui(page_content);
	page_content += F("</div></div>");
	end_html_page_guest(page_content);
	guestSuccessMarkRestartPending();
}
#endif

void SensorWebServer::_webserver_finish_setup() {
	debug_outln_info(F("ws: finish_setup ..."));
	if (!wificonfig_loop) {
		sendHttpRedirectGuest();
		return;
	}
#ifdef ALTRUIST_INSIGHT
	insightGuestClearFinishPending();
#endif
	guestSuccessClearRestartPending();

	RESERVE_STRING(page_content, SMALL_STR);
	start_html_page(page_content, F(INTL_SETUP_COMPLETE), true);
	page_content += F("<div class='guest-page'><div class='guest-card guest-card--connect'>"
		"<div class='guest__connect-status'>"
		"<h2 class='guest__connect-title'>" INTL_GUEST_FINISHING_SETUP "</h2>"
		"</div></div></div>");
	end_html_page_guest(page_content);
	Serial.flush();
	delay(300);
	guestSuccessRestartNow();
}

void SensorWebServer::_webserver_ota() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_OTA_UPDATE), false, "settings");

	String lang_notice;
	if (server.method() == HTTP_POST && server.hasArg("action") && server.arg("action") == F("switch_lang")) {
		String new_lang = server.arg("current_lang");
		new_lang.toUpperCase();
		if (new_lang == String(CURRENT_LANG)) {
			lang_notice = F("<div class='ui-notice ui-notice--warn'><strong>");
			lang_notice += FPSTR(INTL_OTA_LANG_SAME);
			lang_notice += F("</strong></div>");
		} else {
			strncpy(cfg::current_lang, new_lang.c_str(), sizeof(cfg::current_lang) - 1);
			cfg::current_lang[sizeof(cfg::current_lang) - 1] = '\0';
			writeConfig();
			deviceStatus.ota_update_requested = true;
			lang_notice = F("<div class='ui-notice ui-notice--ok'><strong>");
			lang_notice += FPSTR(INTL_OTA_LANG_REQUESTED);
			lang_notice += F("</strong></div>");
		}
	}

	append_app_page_body_start(page_content, F(INTL_PAGE_OTA_INTRO));
	if (lang_notice.length()) {
		page_content += lang_notice;
	}
	page_content += F("<div class='page-form'>");
	webserver_append_ota_section(page_content, deviceStatus, "/ota");
	page_content += F("</div>");

	append_app_page_body_end(page_content);
	end_html_page_app(page_content);
}

static void jsonAppendEscaped(String& out, const String& in) {
	for (size_t i = 0; i < in.length(); ++i) {
		const char c = in[i];
		if (c == '"' || c == '\\') {
			out += '\\';
		}
		if (c == '\n' || c == '\r') {
			continue;
		}
		out += c;
	}
}

void SensorWebServer::_webserver_ota_check_json() {
	if (WiFi.status() != WL_CONNECTED) {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_JSON), F("{\"status\":\"failed\",\"show_install\":false}"));
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	otaCheckForUpdate(deviceStatus);

	String latest;
	String message;
	const char* status = "failed";
	bool show_install = false;
	switch (deviceStatus.ota_check_ui) {
	case device_status_t::OtaCheckUi_UpToDate:
		status = "uptodate";
		message = FPSTR(INTL_OTA_UP_TO_DATE);
		latest = deviceStatus.ota_remote_version[0]
			? String(deviceStatus.ota_remote_version)
			: String(SOFTWARE_VERSION_STR);
		break;
	case device_status_t::OtaCheckUi_Available:
		status = "available";
		message = FPSTR(INTL_OTA_UPDATE_AVAILABLE);
		show_install = true;
		latest = deviceStatus.ota_remote_version[0]
			? String(deviceStatus.ota_remote_version)
			: String(FPSTR(INTL_OTA_UPDATE_AVAILABLE));
		break;
	default:
		status = "failed";
		message = FPSTR(INTL_OTA_CHECK_FAILED);
		break;
	}

	String json = F("{\"status\":\"");
	json += status;
	json += F("\",\"show_install\":");
	json += show_install ? F("true") : F("false");
	json += F(",\"message\":\"");
	jsonAppendEscaped(json, message);
	json += F("\",\"latest\":\"");
	jsonAppendEscaped(json, latest);
	json += F("\",\"last_check\":\"0s\"}");
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json);
}

void SensorWebServer::_webserver_ota_install_json() {
	if (WiFi.status() != WL_CONNECTED) {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_JSON), F("{\"status\":\"failed\",\"show_install\":false}"));
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	deviceStatus.ota_update_requested = true;
	deviceStatus.ota_check_ui = device_status_t::OtaCheckUi_InstallQueued;
	deviceStatus.ota_failed = false;
	deviceStatus.ota_success = false;
	if (deviceStatus.ota_progress_percent < 0) {
		deviceStatus.ota_progress_percent = 0;
	}

	String json = F("{\"status\":\"install\",\"show_install\":false,\"message\":\"");
	jsonAppendEscaped(json, String(FPSTR(INTL_OTA_INSTALL_REQUESTED)));
	json += F("\",\"updating\":\"");
	jsonAppendEscaped(json, String(FPSTR(INTL_OTA_UPDATING)));
	json += F("\"}");
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json);
}

void SensorWebServer::_webserver_ota_progress_json() {
	if (WiFi.status() != WL_CONNECTED) {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_JSON), F("{\"in_progress\":false}"));
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	const bool queued = deviceStatus.ota_update_requested;
	const int percent = deviceStatus.ota_progress_percent < 0 ? 0 : deviceStatus.ota_progress_percent;

	String json = F("{\"in_progress\":");
	json += deviceStatus.ota_in_progress ? F("true") : F("false");
	json += F(",\"queued\":");
	json += queued ? F("true") : F("false");
	json += F(",\"failed\":");
	json += deviceStatus.ota_failed ? F("true") : F("false");
	json += F(",\"success\":");
	json += deviceStatus.ota_success ? F("true") : F("false");
	json += F(",\"percent\":");
	json += String(percent);
	json += '}';
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json);
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

		RESERVE_STRING(page_content, XLARGE_STR);
		page_content.reserve(4096);

		start_html_page(page_content, FPSTR(INTL_CONFIGURATION), false, "settings", true);
		if (wificonfig_loop) {  // scan for wlan ssids
			page_content += FPSTR(WEB_CONFIG_SCRIPT);
			web_page_flush_chunk(page_content, &server);
		}

		if (server.method() == HTTP_GET) {
			const char* sensor_ss58 =
				(robonomics_address.length() > 0) ? robonomics_address.c_str() : nullptr;
			webserver_config_send_body_get(server, page_content, wificonfig_loop, sensors_data, nullptr, 0, sensor_ss58);
		} else {
#ifdef ALTRUIST_INSIGHT
			const bool prev_use_custom_urban = cfg::use_custom_urban;
			const String prev_custom_urban_ip = String(cfg::custom_altruist_urban);
			const String prev_chosen_urban_ip = String(cfg::chosen_altruist_urban);
			const bool prev_standalone = cfg::standalone;
#endif
			webserver_config_send_body_post(server);
			rwsOnConfigOwnerUpdated(robonomics_address);
#ifdef ALTRUIST_INSIGHT
			if (!prev_standalone && cfg::standalone) {
				cfgApplyStandaloneModeEnabled();
			} else if (prev_standalone && !cfg::standalone) {
				cfgOnStandaloneModeDisabled();
			}
			if (prev_use_custom_urban != cfg::use_custom_urban ||
			    prev_custom_urban_ip != String(cfg::custom_altruist_urban) ||
			    prev_chosen_urban_ip != String(cfg::chosen_altruist_urban)) {
				if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
					clearUrbanPairingTelemetry(sensors_data);
					xSemaphoreGive(mutex);
				}
				displayManager.clearUrbanCache();
			}
#endif
			page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
			web_page_flush_chunk(page_content, &server);
		}
		end_html_page_app(page_content);

		if (server.method() == HTTP_POST) {
			if (writeConfig()) {
				if (wificonfig_loop) {
					debug_outln_info(F("ws: config saved during captive portal (no restart)"));
				} else {
					set_restart_reason(RESTART_REASON_CONFIG);
					sensor_restart();
				}
			}
		}
	}
}

void SensorWebServer::_webserver_hub_local() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	const String title = buildLocalAccessLabel();
	start_html_page(page_content, title, false, "local", true);

	if (server.method() == HTTP_POST) {
		if (server.hasArg("action") && server.arg("action") == F("switch_lang")) {
			String new_lang = server.arg("current_lang");
			new_lang.toUpperCase();
			if (new_lang != String(CURRENT_LANG)) {
				strncpy(cfg::current_lang, new_lang.c_str(), sizeof(cfg::current_lang) - 1);
				cfg::current_lang[sizeof(cfg::current_lang) - 1] = '\0';
				writeConfig();
				deviceStatus.ota_update_requested = true;
			}
			DynamicJsonDocument values_snapshot(sensors_data.capacity());
			bool readings_busy = false;
			if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
				readings_busy = true;
			} else {
				values_snapshot.set(sensors_data.as<JsonVariantConst>());
				xSemaphoreGive(mutex);
				if (values_snapshot.overflowed()) {
					readings_busy = true;
				}
			}
			webserver_hub_local(page_content, values_snapshot, deviceStatus, server, wificonfig_loop, readings_busy);
			end_html_page_app(page_content);
			return;
		}
#ifdef ALTRUIST_INSIGHT
		const bool prev_use_custom_urban = cfg::use_custom_urban;
		const String prev_custom_urban_ip = String(cfg::custom_altruist_urban);
		const String prev_chosen_urban_ip = String(cfg::chosen_altruist_urban);
		const bool prev_standalone = cfg::standalone;
#endif
		webserver_config_send_body_post(server);
		rwsOnConfigOwnerUpdated(robonomics_address);
#ifdef ALTRUIST_INSIGHT
		if (!prev_standalone && cfg::standalone) {
			cfgApplyStandaloneModeEnabled();
		} else if (prev_standalone && !cfg::standalone) {
			cfgOnStandaloneModeDisabled();
		}
		if (prev_use_custom_urban != cfg::use_custom_urban ||
		    prev_custom_urban_ip != String(cfg::custom_altruist_urban) ||
		    prev_chosen_urban_ip != String(cfg::chosen_altruist_urban)) {
			if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
				clearUrbanPairingTelemetry(sensors_data);
				xSemaphoreGive(mutex);
			}
			displayManager.clearUrbanCache();
		}
#endif
		page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
		web_page_flush_chunk(page_content, &server);
		end_html_page_app(page_content);
		if (writeConfig()) {
			if (wificonfig_loop) {
				debug_outln_info(F("ws: hub local saved during captive portal (no restart)"));
			} else {
				set_restart_reason(RESTART_REASON_CONFIG);
				sensor_restart();
			}
		}
		return;
	}

	DynamicJsonDocument values_snapshot(sensors_data.capacity());
	bool readings_busy = false;
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		readings_busy = true;
	} else {
		values_snapshot.set(sensors_data.as<JsonVariantConst>());
		xSemaphoreGive(mutex);
		if (values_snapshot.overflowed()) {
			readings_busy = true;
		}
	}

	webserver_hub_local(page_content, values_snapshot, deviceStatus, server, wificonfig_loop, readings_busy);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_social() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DASH_GROUP_SOCIAL_TITLE), false, "social", true);

	const String self_ss58 = String(robonomics.getSs58Address());
	setRobonomicsAddress(self_ss58);
	rwsSyncGroupModeFromOwner(self_ss58);

	RwsGroupApplyResult group_save = RwsGroupApply_None;
	if (server.method() == HTTP_POST) {
		if (server.hasArg(F("save_group"))) {
			group_save = webserver_group_post(server, self_ss58);
		} else {
#ifdef ALTRUIST_INSIGHT
			const bool prev_use_custom_urban = cfg::use_custom_urban;
			const String prev_custom_urban_ip = String(cfg::custom_altruist_urban);
			const String prev_chosen_urban_ip = String(cfg::chosen_altruist_urban);
			const bool prev_standalone = cfg::standalone;
#endif
			webserver_config_send_body_post(server);
			rwsOnConfigOwnerUpdated(robonomics_address);
#ifdef ALTRUIST_INSIGHT
			if (!prev_standalone && cfg::standalone) {
				cfgApplyStandaloneModeEnabled();
			} else if (prev_standalone && !cfg::standalone) {
				cfgOnStandaloneModeDisabled();
			}
			if (prev_use_custom_urban != cfg::use_custom_urban ||
			    prev_custom_urban_ip != String(cfg::custom_altruist_urban) ||
			    prev_chosen_urban_ip != String(cfg::chosen_altruist_urban)) {
				if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
					clearUrbanPairingTelemetry(sensors_data);
					xSemaphoreGive(mutex);
				}
				displayManager.clearUrbanCache();
			}
#endif
			page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
			web_page_flush_chunk(page_content, &server);
			end_html_page_app(page_content);
			if (writeConfig()) {
				set_restart_reason(RESTART_REASON_CONFIG);
				sensor_restart();
			}
			return;
		}
	}

	DynamicJsonDocument values_snapshot(sensors_data.capacity());
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		values_snapshot.clear();
	} else {
		values_snapshot.set(sensors_data.as<JsonVariantConst>());
		xSemaphoreGive(mutex);
	}

	webserver_hub_social(page_content, self_ss58, &robonomics, server, values_snapshot, group_save, wificonfig_loop);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_custom() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DASH_GROUP_CUSTOM_TITLE), false, "custom", true);

	if (server.method() == HTTP_POST) {
		webserver_config_send_body_post(server);
		page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
		web_page_flush_chunk(page_content, &server);
		end_html_page_app(page_content);
		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
		return;
	}

	webserver_hub_custom(page_content, server);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_advanced() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_NAV_ADVANCED), false, "advanced", true);

	if (server.method() == HTTP_POST) {
		webserver_config_send_body_post(server);
		page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
		web_page_flush_chunk(page_content, &server);
		end_html_page_app(page_content);
		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
		return;
	}

	webserver_hub_advanced(page_content, server, wificonfig_loop);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_warnings_redirect() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}
	server.sendHeader(F("Location"), F("/advanced"));
	server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
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

void SensorWebServer::stream_html_page_head(const String& title, bool guest_page, const char* app_page, bool app_config_layout) {
	RESERVE_STRING(s, LARGE_STR);
	s = FPSTR(WEB_PAGE_HEADER);
	s.replace("{t}", title.length() ? title : String(F(INTL_DASH_TITLE)));
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), s);
	yield();

	if (app_page) {
		if (app_config_layout) {
			s = FPSTR(WEB_PAGE_APP_CONFIG_HEADER_HEAD);
		} else {
			s = FPSTR(WEB_PAGE_APP_HEADER_HEAD);
		}
		s.replace(F("{page}"), app_page);
		server.sendContent(s);
		yield();

		s = FPSTR(WEB_PAGE_APP_TOPBAR_BODY);
		fill_app_topbar_placeholders(s, deviceStatus, esp_chipid, robonomics_address);
		server.sendContent(s);
		yield();

		s = FPSTR(WEB_PAGE_APP_LAYOUT_OPEN);
		server.sendContent(s);
		yield();

		RESERVE_STRING(sidebar, LARGE_STR);
		append_app_sidebar(sidebar);
		server.sendContent(sidebar);
		yield();

		if (app_config_layout) {
			s = FPSTR(WEB_PAGE_APP_CONFIG_MAIN_OPEN);
		} else {
			s = FPSTR(WEB_PAGE_APP_MAIN_OPEN);
		}
		s.replace(F("{t}"), title.length() ? title : String(F(INTL_DASH_TITLE)));
		s.replace(F("{home}"), buildLocalAccessLabel());
		server.sendContent(s);
		yield();
		return;
	}

	if (title.indexOf(FPSTR(INTL_CONFIGURATION)) != -1) {
		server.sendContent_P(WEB_PAGE_HEADER_CONFIG_HEAD);
	} else {
		server.sendContent_P(WEB_PAGE_HEADER_HEAD);
	}
	yield();

	if (guest_page) {
		s = FPSTR(WEB_PAGE_GUEST_HEADER_BODY);
	} else if (title.indexOf(FPSTR(INTL_DEBUG_LEVEL)) != -1) {
		s = FPSTR(WEB_PAGE_DEBUG_HEADER_BODY);
	} else if (title.indexOf(FPSTR(INTL_CONFIGURATION)) != -1) {
		s = FPSTR(WEB_PAGE_CONFIG_HEADER_BODY);
	} else if (title.indexOf(FPSTR(INTL_OTA_UPDATE)) != -1 || title.indexOf(FPSTR(INTL_GROUP_MENU)) != -1) {
		s = FPSTR(WEB_PAGE_DATA_HEADER_BODY);
	} else {
		s = FPSTR(WEB_PAGE_HEADER_BODY);
	}
	s.replace("{addr}", robonomics_address);
	if (!guest_page) {
		s.replace("{t}", title);
		if (title != " ") {
			s.replace("{n}", F("&raquo;"));
		} else {
			s.replace("{n}", emptyString);
		}
	}
	s.replace("{id}", esp_chipid);
	s.replace("{mac}", WiFi.macAddress());
	server.sendContent(s);
	yield();
}

void SensorWebServer::start_html_page(String& page_content, const String& title, bool guest_page, const char* app_page, bool app_config_layout) {
	(void)page_content;
	stream_html_page_head(title, guest_page, app_page, app_config_layout);
}

void SensorWebServer::end_html_page(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	server.sendContent_P(WEB_PAGE_FOOTER);
	web_page_finish_chunked(&server);
}

void SensorWebServer::end_html_page_guest(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	server.sendContent_P(WEB_PAGE_GUEST_FOOTER);
	web_page_finish_chunked(&server);
}

void SensorWebServer::end_html_page_root(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	server.sendContent_P(WEB_PAGE_ROOT_FOOTER);
	web_page_finish_chunked(&server);
}

void SensorWebServer::end_html_page_app(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	// Sidebar TOC scroll-spy (#152): highlight the in-view hub card (before </body>).
	server.sendContent(F(
		"<script>(function(){"
		"var page=document.body&&document.body.getAttribute('data-page');"
		"var root=page?document.querySelector('.app-sidebar__sub--'+page):null;"
		"var links=[].slice.call((root||document).querySelectorAll('.app-sidebar__subitem[href*=\"#\"]'));"
		"if(!links.length)return;"
		"var map=[];"
		"for(var i=0;i<links.length;i++){"
		"var h=links[i].getAttribute('href')||'';"
		"var p=h.indexOf('#');"
		"if(p<0)continue;"
		"var id=h.slice(p+1);"
		"if(!id)continue;"
		"var el=document.getElementById(id);"
		"if(el)map.push({a:links[i],el:el,id:id});"
		"}"
		"if(!map.length)return;"
		"function setActive(id){"
		"for(var i=0;i<map.length;i++){"
		"var on=map[i].id===id;"
		"map[i].a.classList.toggle('is-active',on);"
		"if(on)map[i].a.setAttribute('aria-current','true');"
		"else map[i].a.removeAttribute('aria-current');"
		"}"
		"}"
		"function onScroll(){"
		"var cur=map[0].id;"
		"var probe=96;"
		"for(var i=0;i<map.length;i++){"
		"if(map[i].el.getBoundingClientRect().top<=probe)cur=map[i].id;"
		"}"
		"setActive(cur);"
		"}"
		"var hash=(location.hash||'').replace(/^#/,'');"
		"if(hash){for(var i=0;i<map.length;i++){if(map[i].id===hash){setActive(hash);break;}}}"
		"window.addEventListener('scroll',onScroll,{passive:true});"
		"window.addEventListener('hashchange',function(){"
		"var h=(location.hash||'').replace(/^#/,'');"
		"if(h)setActive(h);"
		"});"
		"onScroll();"
		"})();</script>"));
	RESERVE_STRING(footer, XLARGE_STR);
	footer = FPSTR(WEB_PAGE_APP_FOOTER);
	server.sendContent(footer);
	web_page_finish_chunked(&server);
}
