#include "pages.h"
#include "../../utils.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"

/*****************************************************************
 * Webserver wifi: show available wifi networks                  *
 *****************************************************************/

void webserver_wifi(struct_wifiInfo* wifiInfo, uint8_t count_wifiInfo, String &page_content) {

	if (count_wifiInfo == 0 || wifiInfo == nullptr) {
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(INTL_NO_NETWORKS);
		page_content += FPSTR(BR_TAG);
		return;
	}

	if (count_wifiInfo > WIFI_SCAN_LIST_MAX) {
		count_wifiInfo = WIFI_SCAN_LIST_MAX;
	}

	int indices[WIFI_SCAN_LIST_MAX];
	for (unsigned i = 0; i < count_wifiInfo; ++i) {
		indices[i] = static_cast<int>(i);
	}
	for (unsigned i = 0; i < count_wifiInfo; i++) {
		for (unsigned j = i + 1; j < count_wifiInfo; j++) {
			if (wifiInfo[indices[j]].RSSI > wifiInfo[indices[i]].RSSI) {
				std::swap(indices[i], indices[j]);
			}
		}
	}
	int duplicateSsids = 0;
	for (int i = 0; i < count_wifiInfo; i++) {
		if (indices[i] == -1) {
			continue;
		}
		for (int j = i + 1; j < count_wifiInfo; j++) {
			if (strncmp(wifiInfo[indices[i]].ssid, wifiInfo[indices[j]].ssid, sizeof(wifiInfo[0].ssid)) == 0) {
				indices[j] = -1;
				++duplicateSsids;
			}
		}
	}

	page_content += FPSTR(INTL_NETWORKS_FOUND);
	page_content += String(count_wifiInfo - duplicateSsids);
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(TABLE_TAG_OPEN);
	for (int i = 0; i < count_wifiInfo; ++i) {
		if (indices[i] == -1
#if defined (ESP8266)
			|| wifiInfo[indices[i]].isHidden
#endif
		) {
			continue;
		}
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C3)
		append_wlan_ssid_table_row(page_content, wifiInfo[indices[i]].ssid, wifiInfo[indices[i]].encryptionType,
		                           wifiInfo[indices[i]].RSSI);
#endif
	}
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
}
