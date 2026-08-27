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
void webserver_status_part1(String &page_content, device_status_t &deviceStatus, WebServer &server, bool hub_embed) {
	if (!hub_embed) {
		append_app_page_body_start(page_content, F(INTL_PAGE_STATUS_INTRO));
	}
	page_content += F("<div class='data-sheet'>");

	add_data_section_start(page_content, FPSTR(INTL_DATA_SECTION_OVERVIEW));
	add_data_block_intro(page_content, F(INTL_STATUS_SECTION_OVERVIEW_INTRO));
	add_reading_metrics_grid_start(page_content);
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo, 0)) {
		add_reading_metric_card(page_content, FPSTR(INTL_TIME_LOCAL), String(F("—")), nullptr);
	} else {
		char time_str[32];
		strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
		add_reading_metric_card(page_content, FPSTR(INTL_TIME_LOCAL), time_str, nullptr);
	}
	add_reading_metric_card(
		page_content,
		FPSTR(INTL_UPTIME),
		delayToString(millis() - deviceStatus.time_point_device_start_ms),
		nullptr);
	add_reading_metric_card(page_content, FPSTR(INTL_IP_ADDRESS), deviceStatus.ip_address, nullptr);
	add_reading_metric_card(page_content, FPSTR(INTL_RESET_REASON), get_reset_reason_text(), nullptr);
	add_reading_metrics_grid_end(page_content);

	CrashContextStatus crash_ctx = loadCrashContextStatus();
	if (crash_ctx.valid) {
		add_data_row_from_value(page_content, "Last active section", getCrashSectionNameForStatus(crash_ctx.section));
		add_data_row_from_value(page_content, "Free heap before reset", String(crash_ctx.free_heap));
	}
#if defined(ALTRUIST_BUILD_DEBUG)
	add_data_row_from_value(page_content, "Reset reason code", String((int)esp_reset_reason()));
	if (!crash_ctx.valid) {
		add_data_row_from_value(page_content, "Last active section", "N/A (no saved context)");
	}
#endif
	add_data_section_end(page_content);
	web_page_flush_chunk(page_content, &server);

	add_data_section_start(page_content, FPSTR(INTL_DATA_SECTION_DEVICE));
	add_data_block_intro(page_content, F(INTL_STATUS_SECTION_DEVICE_INTRO));
	add_reading_metrics_grid_start(page_content);
	add_reading_metric_card(
		page_content,
		FPSTR(INTL_FIRMWARE),
		String(SOFTWARE_VERSION_STR),
		nullptr);
	add_reading_metric_card(
		page_content,
		FPSTR(INTL_SD_CONNECTED),
		deviceStatus.sd_card_connected ? String(FPSTR(INTL_VALUE_YES)) : String(FPSTR(INTL_VALUE_NO)),
		nullptr);
	add_reading_metric_card(page_content, FPSTR(INTL_FREE_RAM), String(ESP.getFreeHeap()), "B");
	if (cfg::auto_update) {
		add_reading_metric_card(
			page_content,
			FPSTR(INTL_LAST_OTA),
			delayToString(millis() - deviceStatus.last_update_attempt),
			nullptr);
	}
	add_reading_metrics_grid_end(page_content);
	add_data_section_end(page_content);
	web_page_flush_chunk(page_content, &server);

	page_content += F("<div class='data-block data-block--technical'>"
		"<div class='data-block__head'>"
		"<h3 class='data-block__title'>");
	page_content += FPSTR(INTL_DATA_SECTION_TECHNICAL);
	page_content += F("</h3>"
		"<button type='button' id='tech-details-copy' class='encrypt-key-btn encrypt-key-btn--ghost'"
		" data-copied='");
	page_content += FPSTR(INTL_COPIED);
	page_content += F("'>");
	page_content += FPSTR(INTL_COPY_ALL);
	page_content += F("</button></div>"
		"<p class='data-block__intro'>");
	page_content += F(INTL_STATUS_SECTION_TECH_INTRO);
	page_content += F("</p><div class='data-block__rows' id='tech-details-rows'>");
	add_data_row_from_value(page_content, "Firmware channel", ALTRUIST_BUILD_CHANNEL);
	add_data_row_from_value(page_content, "Source commit", ALTRUIST_BUILD_COMMIT);
	add_data_row_from_value(page_content, "Device model", ALTRUIST_BUILD_MODEL);
	add_data_row_from_value(page_content, "ESP target", ALTRUIST_BUILD_TARGET);
	add_data_row_from_value(page_content, "Firmware language", ALTRUIST_BUILD_LANGUAGE);
	add_data_row_from_value(page_content, "Build profile", ALTRUIST_BUILD_PROFILE);
#ifdef CONFIG_IDF_TARGET_ESP32C6
	add_data_row_from_value(page_content, FPSTR(INTL_CHIP_TYPE), "esp32c6");
#endif
#ifdef CONFIG_IDF_TARGET_ESP32C3
	add_data_row_from_value(page_content, FPSTR(INTL_CHIP_TYPE), "esp32c3");
#endif
	add_data_section_end(page_content);
	page_content += F("<script>(function(){"
		"var btn=document.getElementById('tech-details-copy');"
		"var rows=document.getElementById('tech-details-rows');"
		"if(!btn||!rows)return;"
		"function copyText(t){"
		"if(navigator.clipboard&&navigator.clipboard.writeText){"
		"return navigator.clipboard.writeText(t);}"
		"return new Promise(function(resolve,reject){"
		"try{var o=document.createElement('textarea');o.value=t;"
		"o.style.position='fixed';o.style.opacity='0';document.body.appendChild(o);"
		"o.select();document.execCommand('copy');document.body.removeChild(o);resolve();}"
		"catch(e){reject(e);}});}"
		"function gather(){"
		"var lines=[],items=rows.querySelectorAll('.data-line');"
		"for(var i=0;i<items.length;i++){"
		"var n=items[i].querySelector('.data-line__name');"
		"var v=items[i].querySelector('.data-line__val');"
		"if(!n||!v)continue;"
		"lines.push(((n.textContent||'').trim())+': '+((v.textContent||'').trim()));}"
		"return lines.join('\\n');}"
		"btn.addEventListener('click',function(){"
		"var label=btn.getAttribute('data-copied')||'Copied';"
		"var prev=btn.textContent;"
		"copyText(gather()).then(function(){"
		"btn.textContent=label;setTimeout(function(){btn.textContent=prev;},1500);}"
		").catch(function(){});});"
		"})();</script>");
	web_page_flush_chunk(page_content, &server);

#ifdef ALTRUIST_INSIDE
	add_data_section_start(page_content, F("Analytics"));
	add_data_row_from_value(page_content, "Analytics history persistence", analyticsHistoryPersistenceEnabled() ? "ENABLED" : "DISABLED");
	add_data_row_from_value(page_content, "Analytics history loaded", analyticsHistoryIsLoaded() ? String(FPSTR(INTL_VALUE_YES)) : String(FPSTR(INTL_VALUE_NO)));
	add_data_row_from_value(page_content, "Analytics history has data", analyticsHistoryHasData() ? String(FPSTR(INTL_VALUE_YES)) : String(FPSTR(INTL_VALUE_NO)));
	add_data_section_end(page_content);
	web_page_flush_chunk(page_content, &server);
#endif
}

void webserver_status_part2(String &page_content, device_status_t &deviceStatus, WebServer &server) {
	if (deviceStatus.last_update_returncode != 0) {
		add_data_section_start(page_content, FPSTR(INTL_OTA_UPDATE));
		add_data_row_from_value(page_content, FPSTR(INTL_OTA_RETURN),
			deviceStatus.last_update_returncode > 0 ? String(deviceStatus.last_update_returncode) : HTTPClient::errorToString(deviceStatus.last_update_returncode));
		add_data_section_end(page_content);
		web_page_flush_chunk(page_content, &server);
	}
	if (!deviceStatus.apis_status.empty()) {
		add_data_section_start(page_content, FPSTR(INTL_DATA_SECTION_EXPORT), "data-block--export");
		add_data_block_intro(page_content, F(INTL_STATUS_SECTION_EXPORT_INTRO));
		for (const auto& [key, value] : deviceStatus.apis_status) {
			const String api_is_ok = value.is_ok ? "OK" : "ERROR";
			const String api_count_sends = String(value.count_sends_success) + "/" + String(value.count_sends);
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
			add_data_api_status_row(page_content, String(key.c_str()), api_is_ok, api_count_sends, api_last_send);
		}
		add_data_section_end(page_content);
		web_page_flush_chunk(page_content, &server);
	}
}
