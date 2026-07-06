#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include "webserver/webserver.h"

bool connectWifi(SensorWebServer &webserver, bool station_join_already_started = false);
void wifiConfig(SensorWebServer &webserver);

/** Start STA join only (no wait, no mDNS). For Insight boot: call right after readConfig so association runs while e-ink / sensors init. */
void wifiStaBeginStationJoin(void);

/** Thread-safe: user asked for captive portal (Urban reset tap / Insight DOWN 5 s). Consumed in sensor worker. */
void requestWifiConfigPortal(void);
bool wifiTakeUserPortalRequest(void);
bool wifiIsConfigPortalRunning(void);

/** True if user has stored a home WLAN SSID (not factory default / empty). */
bool wifiHasSavedStationCredentials();

/**
 * True when STA has a usable link for LAN/mDNS: connected and non-zero STA IPv4.
 * Avoids treating "WL_CONNECTED" with 0.0.0.0 / stuck DHCP as healthy (breaks recovery timers).
 */
bool wifiStaLinkReady(void);

/** True when STA has joined the home AP during captive portal (not AP-only / 192.168.4.x). */
bool wifiGuestPortalStaReady(void);

/** Abort any in-flight STA join before captive-portal WiFi.begin() (avoids ESP_ERR_WIFI_STATE on retry). */
void wifiGuestPortalPrepareStaJoin(void);

/** Restart after Urban guest success (config must already be saved). Does not return. */
void wifiCaptivePortalRestartAfterSuccess(void);

/** Leave captive portal loop after successful POST (before restart). */
void wifiRequestPortalExit(void);

#if defined(ALTRUIST_INSIGHT)
/** Insight guest WiFi OK but setup step 2 (Continue) not done yet — server-side auto-finish deadline. */
void insightGuestMarkFinishPending(void);
void insightGuestClearFinishPending(void);
/** Call from captive portal loop; applies standalone + restart when deadline elapses. */
void insightGuestProcessPendingFinish(void);
#endif

/**
 * Save config, turn off setup AP, restart immediately (must be called from captive portal POST).
 * Does not return on success.
 */
bool wifiFinishCaptivePortalSaveAndRestart(void);

/**
 * Apply Wi-Fi credentials received via Improv Serial.
 * Copies SSID/password to config, saves to SPIFFS, and attempts connection.
 * Returns true if STA successfully connected with the new credentials.
 */
bool wifiApplyImprovCredentials(const String& ssid, const String& password);

#if defined(ESP32)
/** Register WiFi event hooks (STA disconnect → fast reconnect kick). Call once after WiFi.persistent(). */
void wifiRegisterStaRecoveryEvents(void);
/** True once (debounced) after a STA disconnect event; sensor worker should call wifiStaRuntimeRecovery(false). */
bool wifiStaTakeDisconnectReconnectKick(void);
/**
 * Runtime STA reconnect (outages). Throttled; waits after disconnect so we do not WiFi.begin() while IDF reports "sta is connecting".
 * @param deep_radio_off  If true, may cycle WIFI_OFF (expensive); internally throttled to at most ~1/90s.
 * @return true if a reconnect attempt was started (false if throttled / skipped).
 */
bool wifiStaRuntimeRecovery(bool deep_radio_off);
/**
 * True once (debounced) after STA got IPv4 from DHCP.
 * Re-bind WebServer + mDNS here: after WAN/AP/router recovery the old listener can stay dead until reboot
 * even when `wifiStaLinkReady()` never went false (no edge for the link-ready transition handler).
 */
bool wifiStaTakeStaGotIpWebRefreshKick(void);
#endif

#endif // __WIFI_MANAGER_H__