#include "pages.h"
#include "../../intl.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../defines.h"
#include "../html-content.h"
#include "../../config_manager/config_helpers.h"
#include <HTTPClient.h>

/*****************************************************************
 * Webserver root: show device status
 *****************************************************************/
void webserver_status_part1(String &page_content, device_status_t &deviceStatus) {
	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
			"<thead><tr><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");
	String versionHtml(SOFTWARE_VERSION_STR);
	versionHtml.replace("/", FPSTR(BR_TAG));
	add_table_row_from_value(page_content, FPSTR(INTL_FIRMWARE), versionHtml);
	add_table_row_from_value(page_content, FPSTR(INTL_IP_ADDRESS), deviceStatus.ip_address);
#ifdef CONFIG_IDF_TARGET_ESP32C6
	add_table_row_from_value(page_content, FPSTR(INTL_CHIP_TYPE), "esp32c6");
#endif
#ifdef CONFIG_IDF_TARGET_ESP32C3
	add_table_row_from_value(page_content, FPSTR(INTL_CHIP_TYPE), "esp32c3");
#endif
	add_table_row_from_value(page_content, FPSTR(INTL_SD_CONNECTED), deviceStatus.sd_card_connected ? "YES" : "NO");
	add_table_row_from_value(page_content, FPSTR(INTL_FREE_RAM), String(ESP.getFreeHeap()));
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
}

void webserver_status_part2(String &page_content, device_status_t &deviceStatus) {

	if (deviceStatus.last_update_returncode != 0) {
		add_table_row_from_value(page_content, FPSTR(INTL_OTA_RETURN),
			deviceStatus.last_update_returncode > 0 ? String(deviceStatus.last_update_returncode) : HTTPClient::errorToString(deviceStatus.last_update_returncode));
	}
    for (const auto& [key, value] : deviceStatus.apis_status) {
        String api_is_ok = value.is_ok ? "OK" : "ERROR";
        String api_count_sends = String(value.count_sends_success) + "/" + String(value.count_sends);
        String api_last_send = ctime(&value.last_send_time);
		std::string boldKey = "<b>" + key + "</b>";
        add_table_row_from_value(page_content, boldKey.c_str(), api_is_ok);
        add_table_row_from_value(page_content, FPSTR(INTL_COUNT_SUCCESS_SENDS), api_count_sends);
        add_table_row_from_value(page_content, FPSTR(INTL_LAST_SEND_TIME), api_last_send);
    }
}