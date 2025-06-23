#include "wifi_manager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "utils.h"
#include "config_manager/config_helpers.h"
#include "wifi_info.h"

bool wificonfig_loop;
struct struct_wifiInfo *wifiInfo;
uint8_t count_wifiInfo;

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
static void wifiConfig(SensorWebServer &webserver) {
	debug_outln_info(F("Starting WiFiManager"));
	debug_outln_info(F("AP ID: "), String(cfg::fs_ssid));
	debug_outln_info(F("Password: "), String(cfg::fs_pwd));

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

	WiFi.mode(WIFI_AP);
	const IPAddress apIP(192, 168, 4, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp());
	// In case we create a unique password at first start
	debug_outln_info(F("AP Password is: "), cfg::fs_pwd);

	DNSServer dnsServer;
	// Ensure we don't poison the client DNS cache
	dnsServer.setTTL(0);
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(53, "*", apIP);							// 53 is port for DNS server

	webserver.setup();

	// // 10 minutes timeout for wifi config
	// unsigned long last_page_load = millis();
	unsigned long start_setup_time = millis();
	while (true) {
		dnsServer.processNextRequest();
		webserver.handleClient();
		if (millis() - start_setup_time > 15 * 60 * 1000) {
			debug_outln_error(F("WiFi config timeout, restarting..."));
			esp_restart();
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
}

static void waitForWifiToConnect(int maxRetries) {
	int retryCount = 0;
	while ((WiFi.status() != WL_CONNECTED) && (retryCount < maxRetries)) {
		delay(500);
		debug_out(".", DEBUG_MIN_INFO);
		++retryCount;
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

void connectWifi(SensorWebServer &webserver) {
#if defined(ALTRUIST_URBAN)
	if (WiFi.getAutoConnect()) {
		WiFi.setAutoConnect(false);
	}
#endif
	if (!WiFi.getAutoReconnect()) {
		WiFi.setAutoReconnect(true);
	}

	// Use 13 channels if locale is not "EN"
	wifi_country_t wifi;
	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = (INTL_LANG[0] == 'E' && INTL_LANG[1] == 'N') ? 11 : 13;
	wifi.schan = 1;

#if defined(ESP8266)
	wifi_set_country(&wifi);
#endif

#if defined(ESP32)
	WiFi.setHostname(cfg::fs_ssid);
#endif

	WiFi.mode(WIFI_STA);

#if defined(ESP8266)
	WiFi.hostname(cfg::fs_ssid);
#endif

	if (cfg::wlannopwd) {
		debug_outln_info(F("No password"));
		WiFi.begin(cfg::wlanssid);
	} else {
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
	} // Start WiFI

	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);

	if (strcmp(cfg::wlanssid, WLANSSID) == 0) {
		waitForWifiToConnect(20);
	} else {
		waitForWifiToConnect(120);
	}
	
	debug_outln_info(emptyString);
	if (WiFi.status() != WL_CONNECTED) {
		String fss(cfg::fs_ssid);
		// display_debug(fss.substring(0, 16), fss.substring(16));

		wifi.policy = WIFI_COUNTRY_POLICY_AUTO;

#if defined(ESP8266)
		wifi_set_country(&wifi);
#endif

		wifiConfig(webserver);
		if (WiFi.status() != WL_CONNECTED) {
			waitForWifiToConnect(20);
			debug_outln_info(emptyString);
		}
	}
	debug_outln_info(F("WiFi connected, IP is: "), WiFi.localIP().toString());

	if (MDNS.begin(cfg::local_hostname)) {
		MDNS.addService("altruist", "tcp", 80);
		MDNS.addServiceTxt("altruist", "tcp", "PATH", "/config");
	}
}

