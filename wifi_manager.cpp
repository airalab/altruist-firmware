#include "wifi_manager.h"
#include "config_manager/config_helpers.h"
#include "improv/improv_serial.h"
#include "utils.h"
#include <WiFi.h>
#if defined(ESP32)
#include <esp_netif.h>
#endif
#if !defined(ALTRUIST_URBAN_C3_NO_MDNS)
#include <ESPmDNS.h>
#endif
#include <DNSServer.h>
#include <algorithm>
#include <new>
#include "defines.h"
#include "utils.h"
#include "config_manager/config_helpers.h"
#include "wifi_info.h"
#ifdef ALTRUIST_INSIGHT
#include "display/display_manager.h"
#include "buttons/button_manager.h"
extern DisplayManager displayManager;
extern button_pressed_t btn_press;
#endif

#if defined(ESP32)
// Declared in Arduino-ESP32 WiFiGeneric.cpp; used to update hostname after STA is already up.
esp_netif_t *get_esp_interface_netif(esp_interface_t interface);
#endif

bool wificonfig_loop;
struct struct_wifiInfo *wifiInfo = nullptr;
uint8_t count_wifiInfo;

static struct_wifiInfo s_portal_wifi_cache[WIFI_SCAN_LIST_MAX];
static uint8_t s_portal_wifi_count = 0;
static unsigned long s_last_portal_wifi_scan_ms = 0;
static unsigned long s_portal_loop_started_ms = 0;
static uint8_t s_portal_followup_scans = 0;

static void wifiReadNetworkInfo(uint8_t scan_index, struct_wifiInfo& out) {
	String SSID;
	uint8_t* BSSID = nullptr;
	memset(&out, 0, sizeof(out));
#if defined(ESP8266)
	WiFi.getNetworkInfo(scan_index, SSID, out.encryptionType, out.RSSI, BSSID, out.channel, out.isHidden);
#else
	WiFi.getNetworkInfo(scan_index, SSID, out.encryptionType, out.RSSI, BSSID, (int32_t&)out.channel);
#endif
	SSID.toCharArray(out.ssid, sizeof(out.ssid));
}

static void wifiInsertTopNetwork(struct_wifiInfo* top, uint8_t& top_count, uint8_t top_max, const struct_wifiInfo& candidate) {
	if (top_count < top_max) {
		top[top_count++] = candidate;
		return;
	}
	uint8_t weakest = 0;
	for (uint8_t i = 1; i < top_count; ++i) {
		if (top[i].RSSI < top[weakest].RSSI) {
			weakest = i;
		}
	}
	if (candidate.RSSI > top[weakest].RSSI) {
		top[weakest] = candidate;
	}
}

static void wifiPortalSortAndDedup(void) {
	if (s_portal_wifi_count <= 1) {
		return;
	}
	for (uint8_t i = 0; i + 1 < s_portal_wifi_count; ++i) {
		for (uint8_t j = i + 1; j < s_portal_wifi_count; ++j) {
			if (s_portal_wifi_cache[j].RSSI > s_portal_wifi_cache[i].RSSI) {
				std::swap(s_portal_wifi_cache[i], s_portal_wifi_cache[j]);
			}
		}
	}
	uint8_t unique = 0;
	for (uint8_t i = 0; i < s_portal_wifi_count; ++i) {
#if defined(ESP8266)
		if (s_portal_wifi_cache[i].isHidden) {
			continue;
		}
#endif
		bool duplicate = false;
		for (uint8_t j = 0; j < unique; ++j) {
			if (strncmp(s_portal_wifi_cache[i].ssid, s_portal_wifi_cache[j].ssid,
			            sizeof(s_portal_wifi_cache[0].ssid)) == 0) {
				duplicate = true;
				break;
			}
		}
		if (duplicate) {
			continue;
		}
		if (unique != i) {
			s_portal_wifi_cache[unique] = s_portal_wifi_cache[i];
		}
		++unique;
	}
	s_portal_wifi_count = unique;
}

uint8_t wifiScanInto(struct_wifiInfo* out, uint8_t max_out) {
	if (out == nullptr || max_out == 0) {
		return 0;
	}
	const int8_t scanReturnCode = WiFi.scanNetworks(false /* sync */, true /* hidden */);
	if (scanReturnCode < 0) {
		debug_outln_error(F("WiFi scan failed"));
		logSubsystemError(F("wifi"), F("scan_failed"), String(F("code=")) + String(scanReturnCode));
		return 0;
	}
	uint8_t count = 0;
	for (int8_t i = 0; i < scanReturnCode; ++i) {
		struct_wifiInfo entry;
		wifiReadNetworkInfo(static_cast<uint8_t>(i), entry);
		wifiInsertTopNetwork(out, count, max_out, entry);
	}
	WiFi.scanDelete();
	return count;
}

uint8_t wifiPortalRescan(void) {
	s_portal_wifi_count = wifiScanInto(s_portal_wifi_cache, WIFI_SCAN_LIST_MAX);
	wifiPortalSortAndDedup();
	s_last_portal_wifi_scan_ms = millis();
	return s_portal_wifi_count;
}

void wifiPortalMaybeRescan(void) {
	if (s_portal_loop_started_ms == 0) {
		return;
	}
	if (s_portal_followup_scans == 0 && msSince(s_portal_loop_started_ms) >= 2500UL) {
		wifiPortalRescan();
		s_portal_followup_scans = 1;
		return;
	}
	if (s_portal_followup_scans == 1 && msSince(s_portal_loop_started_ms) >= 6000UL) {
		wifiPortalRescan();
		s_portal_followup_scans = 2;
	}
}

struct_wifiInfo* wifiPortalScanCache(uint8_t* out_count) {
	if (out_count != nullptr) {
		*out_count = s_portal_wifi_count;
	}
	return s_portal_wifi_cache;
}

static volatile bool s_portal_exit_requested = false;

static volatile bool s_user_portal_request = false;

static unsigned long s_guest_success_restart_deadline_ms = 0;

void guestSuccessMarkRestartPending(void) {
	s_guest_success_restart_deadline_ms = millis() + GUEST_SUCCESS_PAGE_DELAY_MS;
}

void guestSuccessClearRestartPending(void) {
	s_guest_success_restart_deadline_ms = 0;
}

void guestSuccessRestartNow(void) {
	s_guest_success_restart_deadline_ms = 0;
	wifiCaptivePortalRestartAfterSuccess();
}

void guestSuccessProcessPendingRestart(void) {
	if (s_guest_success_restart_deadline_ms == 0) {
		return;
	}
	if ((long)(millis() - s_guest_success_restart_deadline_ms) < 0) {
		return;
	}
	s_guest_success_restart_deadline_ms = 0;
	debug_outln_info(F("[WiFi] Guest success: auto-restart after pause"));
	wifiCaptivePortalRestartAfterSuccess();
}

static volatile bool s_improv_provision_restart = false;

void wifiRequestImprovProvisionRestart(void) {
	s_improv_provision_restart = true;
}

void wifiProcessImprovProvisionRestart(void) {
	if (!s_improv_provision_restart) {
		return;
	}
	s_improv_provision_restart = false;
	Serial.flush();
	delay(800);
	if (wificonfig_loop) {
		wifiCaptivePortalRestartAfterSuccess();
	}
	set_restart_reason(RESTART_REASON_CONFIG);
	Serial.flush();
	delay(200);
	esp_restart();
}

#if defined(ESP32)
static volatile bool s_sta_disconnect_pending = false;
static unsigned long s_last_disconnect_kick_ms = 0;

static volatile bool s_sta_got_ip_web_pending = false;
static unsigned long s_last_sta_got_ip_web_kick_ms = 0;

static void wifiStaArduinoEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
	(void)info;
#if defined(ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
	if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
		s_sta_disconnect_pending = true;
	}
#endif
#if defined(ARDUINO_EVENT_WIFI_STA_GOT_IP)
	if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
		s_sta_got_ip_web_pending = true;
	}
#endif
#if !defined(ARDUINO_EVENT_WIFI_STA_DISCONNECTED) && !defined(ARDUINO_EVENT_WIFI_STA_GOT_IP)
	(void)event;
#endif
}

void wifiRegisterStaRecoveryEvents(void) {
#if defined(ARDUINO_EVENT_WIFI_STA_DISCONNECTED) || defined(ARDUINO_EVENT_WIFI_STA_GOT_IP)
	WiFi.onEvent(wifiStaArduinoEvent);
#endif
}

bool wifiStaTakeDisconnectReconnectKick(void) {
	if (!s_sta_disconnect_pending) {
		return false;
	}
	// Coalesce bursts of disconnect events; avoid hammering begin() faster than the stack can handle.
	if (s_last_disconnect_kick_ms != 0 && msSince(s_last_disconnect_kick_ms) < 2000UL) {
		return false;
	}
	s_sta_disconnect_pending = false;
	s_last_disconnect_kick_ms = millis();
	return true;
}

bool wifiStaTakeStaGotIpWebRefreshKick(void) {
	if (!s_sta_got_ip_web_pending) {
		return false;
	}
	// DHCP / lwIP can emit several GOT_IP-ish phases; one listener rebind is enough.
	if (s_last_sta_got_ip_web_kick_ms != 0 && msSince(s_last_sta_got_ip_web_kick_ms) < 1500UL) {
		return false;
	}
	s_sta_got_ip_web_pending = false;
	s_last_sta_got_ip_web_kick_ms = millis();
	return true;
}

bool wifiStaRuntimeRecovery(bool deep_radio_off) {
	if (!wifiHasSavedStationCredentials() || wifiIsConfigPortalRunning()) {
		return false;
	}
	static unsigned long s_last_recover_ms = 0;
	// avoid fighting an ongoing association/DHCP attempt.
	if (s_last_recover_ms != 0 && msSince(s_last_recover_ms) < WIFI_STA_RECOVERY_GRACE_MS) {
		return false;
	}

	// WL_CONNECTED can precede DHCP; tearing down every N seconds prevents ever getting an IP.
	static unsigned long s_assoc_no_ip_since_ms = 0;
	if (WiFi.status() == WL_CONNECTED && WiFi.localIP()[0] == 0) {
		if (s_assoc_no_ip_since_ms == 0) {
			s_assoc_no_ip_since_ms = millis();
		}
		if (msSince(s_assoc_no_ip_since_ms) < WIFI_STA_DHCP_GRACE_MS) {
			s_last_recover_ms = millis();
			return false;
		}
		s_assoc_no_ip_since_ms = 0;
	} else {
		s_assoc_no_ip_since_ms = 0;
	}

	static unsigned long s_last_deep_ms = 0;
	static uint8_t s_deep_throttle_stalls = 0;

	if (!deep_radio_off) {
		s_deep_throttle_stalls = 0;
	}

	bool do_deep = deep_radio_off;
	if (do_deep && s_last_deep_ms != 0 && msSince(s_last_deep_ms) < WIFI_STA_DEEP_RADIO_MIN_INTERVAL_MS) {
		s_deep_throttle_stalls++;
		if (s_deep_throttle_stalls >= WIFI_STA_DEEP_FORCE_AFTER_THROTTLED_SKIPS) {
			s_deep_throttle_stalls = 0;
			debug_outln_info(F("[WiFi] Deep STA recovery forced (radio-off was throttled too long)"));
		} else {
			debug_outln_info(F("[WiFi] Deep STA recovery skipped (radio-off throttled)"));
			s_last_recover_ms = millis();
			return false;
		}
	} else if (do_deep) {
		s_deep_throttle_stalls = 0;
	}
	if (do_deep) {
		s_last_deep_ms = millis();
		debug_outln_info(F("[WiFi] Deep STA recovery (radio off)"));
	} else {
		debug_outln_info(F("[WiFi] Periodic STA recovery (link not ready)"));
	}
	debug_outln_info(F("[WiFi] status="), String((int)WiFi.status()) + F(" ip=") + WiFi.localIP().toString());
	logSubsystemEvent(
		F("event"),
		F("wifi"),
		F("sta_recovery"),
		String(F("mode=")) + (do_deep ? F("deep") : F("soft")) + F(" status=") + String((int)WiFi.status()) +
			F(" ip=") + WiFi.localIP().toString()
	);

	if (do_deep) {
		// Deep recovery: force a clean wifi state transition, then re-issue begin().
		WiFi.disconnect(false, false);
		delay(650);
		WiFi.mode(WIFI_OFF);
		delay(1200);
		wifiApplyStaHostname();
		WiFi.mode(WIFI_STA);
		WiFi.setSleep(false);
		wifiApplyStaHostname();

		if (cfg::wlannopwd) {
			WiFi.begin(cfg::wlanssid);
		} else {
			WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
		}
	} else {
		// Soft recovery: avoid WiFi.begin() while STA may still be connecting (can trigger ESP_ERR_WIFI_STATE).
		// Rely on Arduino-ESP32 auto reconnect + reconnect() kick.
		wifiApplyStaHostname();
		WiFi.mode(WIFI_STA);
		WiFi.setSleep(false);
		WiFi.reconnect();
	}
	s_last_recover_ms = millis();
	return true;
}
#endif // ESP32

void requestWifiConfigPortal(void) {
	s_user_portal_request = true;
}

bool wifiTakeUserPortalRequest(void) {
	if (!s_user_portal_request) {
		return false;
	}
	s_user_portal_request = false;
	return true;
}

bool wifiIsConfigPortalRunning(void) {
	return wificonfig_loop;
}

bool wifiHasSavedStationCredentials() {
	if (cfg::wlanssid[0] == '\0') {
		return false;
	}
	return strcmp(cfg::wlanssid, WLANSSID) != 0;
}

bool wifiStaLinkReady(void) {
#if defined(ESP32) || defined(ESP8266)
	if (WiFi.status() != WL_CONNECTED) {
		return false;
	}
	if (WiFi.localIP()[0] == 0) {
		return false;
	}
	// Do not require gatewayIP(): on some APs / lwIP timing it stays 0 while STA IPv4 is already valid, which made
	// the firmware treat WiFi as "down" forever (recovery loops, Insight skipped Urban, LAN looked dead).
	return true;
#else
	return false;
#endif
}

bool wifiGuestPortalStaReady(void) {
	if (!wifiStaLinkReady()) {
		return false;
	}
	const IPAddress ip = WiFi.localIP();
	// Captive portal AP is 192.168.4.0/24 — do not treat the AP address as home WiFi success.
	if (ip[0] == 192 && ip[1] == 168 && ip[2] == 4) {
		return false;
	}
	return true;
}

void wifiApplyStaHostname(void) {
	char host[32];
	char fallback[32];
	const char* src = cfg::local_hostname;
	if (src == nullptr || src[0] == '\0') {
		cfg::formatDefaultLocalHostname(fallback, sizeof(fallback), get_chipid().c_str());
		src = fallback;
	}

	size_t out = 0;
	for (size_t i = 0; src[i] != '\0' && out + 1 < sizeof(host); ++i) {
		char c = src[i];
		if (c >= 'A' && c <= 'Z') {
			c = static_cast<char>(c - 'A' + 'a');
		}
		const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
		if (ok) {
			host[out++] = c;
		} else if ((c == '.' || c == '_' || c == ' ') && out > 0 && host[out - 1] != '-') {
			host[out++] = '-';
		}
	}
	while (out > 0 && host[out - 1] == '-') {
		--out;
	}
	host[out] = '\0';
	if (out == 0) {
		cfg::formatDefaultLocalHostname(host, sizeof(host), get_chipid().c_str());
	}

#if defined(ESP32)
	// NetworkManager copies into its own buffer (default is esp32c6-XXXXXX until we override).
	WiFi.setHostname(host);

	// Arduino only pushes NetworkManager hostname into esp_netif when STA is first enabled.
	// Captive portal / recovery often already have STA up (AP_STA), so apply directly too.
	esp_netif_t *netif = get_esp_interface_netif(ESP_IF_WIFI_STA);
	if (!netif) {
		netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	}
	if (netif) {
		const esp_err_t err = esp_netif_set_hostname(netif, host);
		if (err != ESP_OK) {
			debug_outln_error(F("[WiFi] esp_netif_set_hostname failed"));
			debug_outln_info(F("[WiFi] esp_netif_set_hostname err="), String((int)err));
		}
	}

	const char *applied = WiFi.getHostname();
	debug_outln_info(F("[WiFi] STA hostname"), String(applied ? applied : host));
#elif defined(ESP8266)
	WiFi.hostname(host);
	debug_outln_info(F("[WiFi] STA hostname"), String(host));
#endif
}

void wifiGuestPortalPrepareStaJoin(void) {
#if defined(ESP32) || defined(ESP8266)
	debug_outln_info(F("[WiFi] Guest portal: preparing STA join"));
	WiFi.disconnect(true, false);
	const unsigned long deadline = millis() + 1200UL;
	while (millis() < deadline) {
		yield();
		delay(50);
		if (WiFi.status() == WL_DISCONNECTED) {
			break;
		}
	}
	delay(80);
	if (WiFi.getMode() != WIFI_AP_STA) {
		WiFi.mode(WIFI_AP_STA);
	}
	delay(50);
#endif
}

void wifiCaptivePortalRestartAfterSuccess(void) {
	wifiRequestPortalExit();
	set_restart_reason(RESTART_REASON_CONFIG);
	debug_outln_info(F("[WiFi] Captive portal: restart after success page"));
#if defined(ESP32) || defined(ESP8266)
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_STA);
#endif
	wificonfig_loop = false;
	Serial.flush();
	delay(400);
	esp_restart();
}

void wifiRequestPortalExit(void) {
	s_portal_exit_requested = true;
}

bool wifiFinishCaptivePortalSaveAndRestart(void) {
	if (!writeConfig()) {
		debug_outln_error(F("[WiFi] Captive portal: failed to save config.json"));
		logSubsystemError(F("wifi"), F("portal_config_save_failed"));
		return false;
	}
	wifiRequestPortalExit();
	set_restart_reason(RESTART_REASON_CONFIG);
	debug_outln_info(F("[WiFi] Config saved; stopping setup AP and restarting"));
#if defined(ESP32) || defined(ESP8266)
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_STA);
#endif
	wificonfig_loop = false;
	Serial.flush();
	delay(400);
	esp_restart();
	return false;
}

static int selectChannelForAp() {
	std::array<int, 14> channels_rssi;
	std::fill(channels_rssi.begin(), channels_rssi.end(), -100);

	for (unsigned i = 0; i < std::min((uint8_t) 14, count_wifiInfo); i++) {
		if (wifiInfo[i].RSSI > channels_rssi[wifiInfo[i].channel]) {
			channels_rssi[wifiInfo[i].channel] = wifiInfo[i].RSSI;
		}
	}

	if ((channels_rssi[1] < channels_rssi[6]) && (channels_rssi[1] < channels_rssi[11])) {
		return 1;
	} else if ((channels_rssi[6] < channels_rssi[1]) && (channels_rssi[6] < channels_rssi[11])) {
		return 6;
	} else {
		return 11;
	}
}

/*****************************************************************
 * WifiConfig                                                    *
 *****************************************************************/
void wifiConfig(SensorWebServer &webserver) {
	debug_outln_info(F("Starting WiFiManager"));
	debug_outln_info(F("AP ID: "), String(cfg::fs_ssid));
	debug_outln_info(F("Password: "), String(cfg::fs_pwd));

	// Track portal state in the wifi module too (used by LED policy).
	wificonfig_loop = true;
	s_portal_exit_requested = false;
	guestSuccessClearRestartPending();
	webserver.setWifiConfigLoop(true);

	WiFi.disconnect(true);

	// Apply hostname before AP_STA so the STA netif is created with altruist-* (not esp32c6-*).
	wifiApplyStaHostname();
	WiFi.mode(WIFI_AP_STA);
	wifiApplyStaHostname();
	const IPAddress apIP(192, 168, 4, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

	debug_outln_info(F("scan for wifi networks..."));
	wifiPortalRescan();
	count_wifiInfo = s_portal_wifi_count;
	wifiInfo = wifiPortalScanCache(nullptr);

	WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp());
	delay(150);
	yield();
	wifiPortalRescan();
	count_wifiInfo = s_portal_wifi_count;
	wifiInfo = wifiPortalScanCache(nullptr);
	webserver.setWifiInfo(wifiInfo, count_wifiInfo);
	// WiFi.softAP(cfg::fs_ssid);
	// In case we create a unique password at first start
	debug_outln_info(F("AP Password is: "), cfg::fs_pwd);

	DNSServer dnsServer;
	// Ensure we don't poison the client DNS cache
	dnsServer.setTTL(0);
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(53, "*", apIP);							// 53 is port for DNS server

	webserver.setup();

	s_portal_loop_started_ms = millis();
	s_portal_followup_scans = 0;

#ifdef ALTRUIST_INSIGHT
	// Full e-ink refresh is slow; defer until AP + DNS + webserver are ready so phones can associate sooner (closer to Urban).
	displayManager.setScreen(ScreenPage::SETUP);
	displayManager.process(btn_press);
#endif

	// // 10 minutes timeout for wifi config
	// unsigned long last_page_load = millis();
	unsigned long start_setup_time = millis();
	while (true) {
		dnsServer.processNextRequest();
		wifiPortalMaybeRescan();
		webserver.handleClient();
		improv_serial_loop();
		wifiProcessImprovProvisionRestart();
		guestSuccessProcessPendingRestart();
#ifdef ALTRUIST_INSIGHT
		insightGuestProcessPendingFinish();
		static unsigned long last_portal_display_ms = 0;
		const unsigned long portal_now = millis();
		if (portal_now - last_portal_display_ms >= 500UL) {
			last_portal_display_ms = portal_now;
			displayManager.process(btn_press);
		}
#endif
		if (millis() - start_setup_time > 15 * 60 * 1000) {
			debug_outln_error(F("WiFi config timeout, restarting..."));
			logSubsystemError(F("wifi"), F("config_timeout"));
			esp_restart();
		}
		if (s_portal_exit_requested) {
			debug_outln_info(F("WiFi config portal: credentials saved, leaving portal"));
			break;
		}
#if defined(ESP8266)
		wdt_reset(); // nodemcu is alive
		MDNS.update();
#endif
		yield();
	}

	WiFi.softAPdisconnect(true);
	dnsServer.stop();
	delay(100);

	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);

	WiFi.mode(WIFI_OFF);
	delay(80);
	wifiApplyStaHostname();
	WiFi.mode(WIFI_STA);
	wifiApplyStaHostname();
	if (cfg::wlannopwd) {
		debug_outln_info(F("No password"));
		WiFi.begin(cfg::wlanssid);
	} else {
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
	}

	debug_outln_info(F("---- Result Webconfig ----"));
	debug_outln_info(F("WLANSSID: "), cfg::wlanssid);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("CSV: "), cfg::send2csv);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("Autoupdate: "), cfg::auto_update);
	// debug_outln_info_bool(F("Display: "), cfg::has_display);
	// debug_outln_info_bool(F("LCD 1602: "), !!lcd_1602);
	debug_outln_info(F("Debug: "), String(cfg::debug));
	webserver.setWifiConfigLoop(false);
	wificonfig_loop = false;
}

// First link check is immediate; then interval_ms between polls (no fixed 500 ms blind wait).
// log_progress must stay off during Improv: '.' on USB CDC shares the Improv Serial stream.
static void waitForWifiToConnect(unsigned maxDelays, unsigned long interval_ms, bool log_progress = true) {
	unsigned delays_done = 0;
	for (;;) {
		improv_serial_loop();
		wifiProcessImprovProvisionRestart();
		if (wifiStaLinkReady()) {
			return;
		}
		if (delays_done >= maxDelays) {
			return;
		}
		delay(interval_ms);
		if (log_progress) {
			debug_out(".", DEBUG_MIN_INFO);
		}
		++delays_done;
	}
}

/*****************************************************************
 * WiFi auto connecting script                                   *
 *****************************************************************/

#if defined(ESP8266)
static WiFiEventHandler disconnectEventHandler;
#endif
#if defined(ESP32)
static WiFiEventId_t disconnectEventHandler;
#endif

static void wifiApplyStaJoinStart(void) {
#if defined(CONFIG_IDF_TARGET_ESP32C3) && defined(ALTRUIST_HAS_WIFI_AUTOCONNECT_API)
	// Some Arduino-ESP32 cores provide get/setAutoConnect, others don't.
	// Keep this optional so ESP32-C3 builds don't break on cores without it.
	if (WiFi.getAutoConnect()) {
		WiFi.setAutoConnect(false);
	}
#endif
	if (!WiFi.getAutoReconnect()) {
		WiFi.setAutoReconnect(true);
	}

#if defined(ESP8266)
	wifi_country_t wifi;
	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = (INTL_LANG[0] == 'E' && INTL_LANG[1] == 'N') ? 11 : 13;
	wifi.schan = 1;
	wifi_set_country(&wifi);
#endif

	// Must run before enabling STA: Arduino applies NetworkManager hostname only on STA enable.
	const wifi_mode_t cm = WiFi.getMode();
	if (cm != WIFI_OFF) {
		WiFi.mode(WIFI_OFF);
		delay(80);
	}
	wifiApplyStaHostname();
	WiFi.mode(WIFI_STA);
#if defined(ESP32)
	// Avoid modem sleep during association; some routers/APs otherwise look "slow" or flaky after outages.
	WiFi.setSleep(false);
	wifiApplyStaHostname();
#endif

	if (cfg::wlannopwd) {
		debug_outln_info(F("No password"));
		WiFi.begin(cfg::wlanssid);
	} else {
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
	}

	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);
}

void wifiStaBeginStationJoin(void) {
	wifiApplyStaJoinStart();
}

bool connectWifi(SensorWebServer &webserver, bool station_join_already_started) {
	(void)webserver;
	if (!station_join_already_started) {
		wifiApplyStaJoinStart();
	}

	// Bounded wait on boot; if STA fails and credentials are saved, setup() skips AP and relies on runtime reconnect.
	// 200 ms * N ≈ previous 500 ms * (N/2.5); first loop iteration checks immediately in waitForWifiToConnect.
#if defined(ALTRUIST_INSIGHT)
	// Insight: ~10 s cap (50 * 200 ms), same order of magnitude as old 20 * 500 ms; worker reconnect handles slow DHCP.
	waitForWifiToConnect(50, 200);
#else
	waitForWifiToConnect(75, 200);
#endif

	debug_outln_info(emptyString);
	if (!wifiStaLinkReady()) {
		return false;
	}
	debug_outln_info(F("WiFi connected, IP is: "), WiFi.localIP().toString());

#if !defined(ALTRUIST_URBAN_C3_NO_MDNS)
	if (MDNS.begin(cfg::local_hostname)) {
		MDNS.addService("altruist", "tcp", 80);
		MDNS.addServiceTxt("altruist", "tcp", "PATH", "/config");
		MDNS.addServiceTxt("altruist", "tcp", DEVICE_MODEL_MDNS_PROPERTY, DEVICE_MODEL);
	}
#endif
	return true;
}

bool wifiApplyImprovCredentials(const String& ssid, const String& password) {
	if (ssid.length() == 0 || ssid.length() >= LEN_WLANSSID) {
		return false;
	}
	if (password.length() >= LEN_CFG_PASSWORD) {
		return false;
	}
	ssid.toCharArray(cfg::wlanssid, LEN_WLANSSID);
	password.toCharArray(cfg::wlanpwd, LEN_CFG_PASSWORD);
	cfg::wlannopwd = (password.length() == 0);
	writeConfig();

	debug_outln_info(F("[IMPROV] Connecting to SSID: "), ssid);

	// Webflasher Improv usually runs while the captive portal AP is up. WIFI_OFF there
	// tears the radio down and burns most of the old ~15 s wait before DHCP can finish.
	const bool keep_setup_ap = (WiFi.getMode() & WIFI_AP) != 0;
	if (keep_setup_ap) {
		wifiGuestPortalPrepareStaJoin();
		wifiApplyStaHostname();
	} else {
		WiFi.disconnect(true, false);
		delay(150);
		WiFi.mode(WIFI_OFF);
		delay(80);
		wifiApplyStaHostname();
		WiFi.mode(WIFI_STA);
		wifiApplyStaHostname();
	}
#if defined(ESP32)
	WiFi.setSleep(false);
#endif
	if (cfg::wlannopwd) {
		WiFi.begin(cfg::wlanssid);
	} else {
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
	}

	// ESP Web Tools waits ~45 s for the WIFI_SETTINGS RPC result. Stay under that
	// and require a real STA IPv4 (not the 192.168.4.x setup AP).
	const unsigned wait_steps = 150; // 150 * 200 ms = 30 s
	unsigned delays_done = 0;
	for (;;) {
		improv_serial_loop();
		const bool ready = keep_setup_ap ? wifiGuestPortalStaReady() : wifiStaLinkReady();
		if (ready) {
			break;
		}
		if (delays_done >= wait_steps) {
			break;
		}
		delay(200);
		++delays_done;
	}
	bool connected = keep_setup_ap ? wifiGuestPortalStaReady() : wifiStaLinkReady();
	if (connected) {
		debug_outln_info(F("[IMPROV] Connected, IP: "), WiFi.localIP().toString());
	} else {
		debug_outln_error(F("[IMPROV] Failed to connect"));
		logSubsystemError(F("wifi"), F("improv_connect_failed"), String(F("status=")) + String((int)WiFi.status()));
	}
	return connected;
}
