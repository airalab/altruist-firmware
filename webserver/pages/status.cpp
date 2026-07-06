#include "pages.h"
#include "../../intl.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../defines.h"
#include "../html-content.h"
#include "../../config_manager/config_helpers.h"
#ifdef ALTRUIST_INSIGHT
#include "../../display/screens/analytics.h"
#endif
#include <HTTPClient.h>
#include <Preferences.h>
#include <esp_system.h>

namespace {
struct CrashContextStatus {
	bool valid = false;
	uint8_t section = 0;
	uint32_t uptime_sec = 0;
	uint32_t free_heap = 0;
};

CrashContextStatus loadCrashContextStatus() {
	CrashContextStatus ctx;
	Preferences prefs;
	prefs.begin("crash", true);
	ctx.valid = prefs.getBool("valid", false);
	if (ctx.valid) {
		ctx.section = prefs.getUChar("section", 0);
		ctx.uptime_sec = prefs.getULong("uptime", 0);
		ctx.free_heap = prefs.getULong("heap", 0);
	}
	prefs.end();
	return ctx;
}

const char* getCrashSectionNameForStatus(uint8_t section) {
	switch (section) {
		case 0: return "Idle/MainLoop";
		case 1: return "FetchSensors";
		case 2: return "RobonomicsDatalog";
		case 3: return "RobonomicsHTTPMap";
		case 4: return "CustomHTTP";
		case 5: return "DisplayUpdate";
		case 6: return "LEDUpdate";
		case 7: return "WiFiReconnect";
		case 8: return "SDWrite";
		default: return "Unknown";
	}
}
} // namespace

/*****************************************************************
 * Webserver root: show device status
 *****************************************************************/
void webserver_status_part1(String &page_content, device_status_t &deviceStatus) {
	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
			"<thead><tr><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");
	String versionHtml(SOFTWARE_VERSION_STR);
	versionHtml.replace("/", FPSTR(BR_TAG));
	add_table_row_from_value(page_content, FPSTR(INTL_FIRMWARE), versionHtml);
	add_table_row_from_value(page_content, "Firmware channel", ALTRUIST_BUILD_CHANNEL);
	add_table_row_from_value(page_content, "Source commit", ALTRUIST_BUILD_COMMIT);
	add_table_row_from_value(page_content, "Device model", ALTRUIST_BUILD_MODEL);
	add_table_row_from_value(page_content, "ESP target", ALTRUIST_BUILD_TARGET);
	add_table_row_from_value(page_content, "Firmware language", ALTRUIST_BUILD_LANGUAGE);
	add_table_row_from_value(page_content, "Build profile", ALTRUIST_BUILD_PROFILE);
	add_table_row_from_value(page_content, FPSTR(INTL_IP_ADDRESS), deviceStatus.ip_address);
	add_table_row_from_value(page_content, FPSTR(INTL_SD_CONNECTED), deviceStatus.sd_card_connected ? "YES" : "NO");
	add_table_row_from_value(page_content, FPSTR(INTL_FREE_RAM), String(ESP.getFreeHeap()));
#ifdef ALTRUIST_INSIGHT
	add_table_row_from_value(page_content, "Analytics history persistence", analyticsHistoryPersistenceEnabled() ? "ENABLED" : "DISABLED");
	add_table_row_from_value(page_content, "Analytics history loaded", analyticsHistoryIsLoaded() ? "YES" : "NO");
	add_table_row_from_value(page_content, "Analytics history has data", analyticsHistoryHasData() ? "YES" : "NO");
#endif
	if (cfg::auto_update) {
		add_table_row_from_value(page_content, FPSTR(INTL_LAST_OTA), delayToString(millis() - deviceStatus.last_update_attempt));
	}

	struct tm timeinfo;
	if (!getLocalTime(&timeinfo)) {
		add_table_row_from_value(page_content, FPSTR(INTL_TIME_LOCAL), "Failed to get time");
	} else {
		char time_str[32];
		strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
		add_table_row_from_value(page_content, FPSTR(INTL_TIME_LOCAL), time_str);
	}
	add_table_row_from_value(page_content, FPSTR(INTL_UPTIME), delayToString(millis() - deviceStatus.time_point_device_start_ms));
	add_table_row_from_value(page_content, FPSTR(INTL_RESET_REASON), get_reset_reason_text());
#if defined(ALTRUIST_BUILD_DEBUG)
	add_table_row_from_value(page_content, "Reset reason code", String((int)esp_reset_reason()));

	CrashContextStatus crash_ctx = loadCrashContextStatus();
	if (crash_ctx.valid) {
		add_table_row_from_value(page_content, "Last crash section", getCrashSectionNameForStatus(crash_ctx.section));
		add_table_row_from_value(page_content, "Prev uptime before reset (s)", String(crash_ctx.uptime_sec));
		add_table_row_from_value(page_content, "Prev free heap before reset (bytes)", String(crash_ctx.free_heap));
	} else {
		add_table_row_from_value(page_content, "Last crash section", "N/A (no saved crash context)");
	}
#endif
}

void webserver_status_part2(String &page_content, device_status_t &deviceStatus) {

	if (deviceStatus.last_update_returncode != 0) {
		add_table_row_from_value(page_content, FPSTR(INTL_OTA_RETURN),
			deviceStatus.last_update_returncode > 0 ? String(deviceStatus.last_update_returncode) : HTTPClient::errorToString(deviceStatus.last_update_returncode));
	}
    for (const auto& [key, value] : deviceStatus.apis_status) {
        String api_is_ok = value.is_ok ? "OK" : "ERROR";
        String api_count_sends = String(value.count_sends_success) + "/" + String(value.count_sends);
        String api_last_send;
        if (value.last_send_time == 0) {
            api_last_send = "N/A";
        } else {
            struct tm ti;
            localtime_r(&value.last_send_time, &ti);
            char buf[24];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
            api_last_send = buf;
        }
		std::string boldKey = "<b>" + key + "</b>";
        add_table_row_from_value(page_content, boldKey.c_str(), api_is_ok);
        add_table_row_from_value(page_content, FPSTR(INTL_COUNT_SUCCESS_SENDS), api_count_sends);
        add_table_row_from_value(page_content, FPSTR(INTL_LAST_SEND_TIME), api_last_send);
    }
}
