#include "wifi_manager.h"
#include "config_manager/config_helpers.h"
#include "improv/improv_serial.h"
#include "utils.h"
#include <WiFi.h>
#if !defined(ALTRUIST_URBAN_C3_NO_MDNS)
#include <ESPmDNS.h>
#endif
#include <DNSServer.h>
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

bool wificonfig_loop;
struct struct_wifiInfo *wifiInfo = nullptr;
uint8_t count_wifiInfo;

static volatile bool s_portal_exit_requested = false;

static volatile bool s_user_portal_request = false;

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

	if (do_deep) {
		// Deep recovery: force a clean wifi state transition, then re-issue begin().
		WiFi.disconnect(false, false);
		delay(650);
		WiFi.mode(WIFI_OFF);
		delay(1200);
		WiFi.mode(WIFI_STA);
		WiFi.setSleep(false);

		if (cfg::wlannopwd) {
			WiFi.begin(cfg::wlanssid);
		} else {
			WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
		}
	} else {
		// Soft recovery: avoid WiFi.begin() while STA may still be connecting (can trigger ESP_ERR_WIFI_STATE).
		// Rely on Arduino-ESP32 auto reconnect + reconnect() kick.
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

void wifiGuestPortalPrepareStaJoin(void) {
#if defined(ESP32) || defined(ESP8266)
	debug_outln_info(F("[WiFi] Guest portal: preparing STA join"));
	WiFi.disconnect(true, false);
	const unsigned long deadline = millis() + 3000UL;
	while (millis() < deadline) {
		yield();
		delay(50);
		if (WiFi.status() == WL_DISCONNECTED) {
			break;
		}
	}
	delay(150);
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
	webserver.setWifiConfigLoop(true);

	WiFi.disconnect(true);
	debug_outln_info(F("scan for wifi networks..."));
	int8_t scanReturnCode = WiFi.scanNetworks(false /* scan async */, true /* show hidden networks */);
	if (scanReturnCode < 0) {
		debug_outln_error(F("WiFi scan failed. Treating as empty. "));
		count_wifiInfo = 0;
	}
	else {
		count_wifiInfo = (uint8_t) scanReturnCode;
	}

	delete [] wifiInfo;
	wifiInfo = new struct_wifiInfo[std::max(count_wifiInfo, (uint8_t) 1)];

	for (unsigned i = 0; i < count_wifiInfo; i++) {
		String SSID;
		uint8_t* BSSID;

		memset(&wifiInfo[i], 0, sizeof(struct_wifiInfo));
#if defined(ESP8266)
		WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType,
			wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel,
			wifiInfo[i].isHidden);
#else
		WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType,
			wifiInfo[i].RSSI, BSSID, (int32_t&)wifiInfo[i].channel);
#endif
		SSID.toCharArray(wifiInfo[i].ssid, sizeof(wifiInfo[0].ssid));
	}

    webserver.setWifiInfo(wifiInfo, count_wifiInfo);

	WiFi.mode(WIFI_AP_STA);
	const IPAddress apIP(192, 168, 4, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp());
	// WiFi.softAP(cfg::fs_ssid);
	// In case we create a unique password at first start
	debug_outln_info(F("AP Password is: "), cfg::fs_pwd);

	DNSServer dnsServer;
	// Ensure we don't poison the client DNS cache
	dnsServer.setTTL(0);
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(53, "*", apIP);							// 53 is port for DNS server

	webserver.setup();

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
		webserver.handleClient();
		improv_serial_loop();
#ifdef ALTRUIST_INSIGHT
		insightGuestProcessPendingFinish();
		// Process display manager to handle button presses (e.g., sleep mode) even during WiFi config
		displayManager.process(btn_press);
#endif
		if (millis() - start_setup_time > 15 * 60 * 1000) {
			debug_outln_error(F("WiFi config timeout, restarting..."));
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
	WiFi.mode(WIFI_STA);

	dnsServer.stop();
	delay(100);

	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);

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
static void waitForWifiToConnect(unsigned maxDelays, unsigned long interval_ms) {
	unsigned delays_done = 0;
	for (;;) {
		improv_serial_loop();
		if (wifiStaLinkReady()) {
			return;
		}
		if (delays_done >= maxDelays) {
			return;
		}
		delay(interval_ms);
		debug_out(".", DEBUG_MIN_INFO);
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

#if defined(ESP32)
	WiFi.setHostname(cfg::fs_ssid);
#endif

	WiFi.mode(WIFI_STA);
#if defined(ESP32)
	// Avoid modem sleep during association; some routers/APs otherwise look "slow" or flaky after outages.
	WiFi.setSleep(false);
#endif

#if defined(ESP8266)
	WiFi.hostname(cfg::fs_ssid);
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

	WiFi.disconnect(true, false);
	delay(150);
	WiFi.mode(WIFI_STA);
	WiFi.setSleep(false);
	if (cfg::wlannopwd) {
		WiFi.begin(cfg::wlanssid);
	} else {
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
	}

	waitForWifiToConnect(75, 200);
	bool connected = wifiStaLinkReady();
	if (connected) {
		debug_outln_info(F("[IMPROV] Connected, IP: "), WiFi.localIP().toString());
	} else {
		debug_outln_error(F("[IMPROV] Failed to connect"));
	}
	return connected;
}

