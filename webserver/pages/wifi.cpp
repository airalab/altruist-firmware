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

	debug_outln_info(F("wifi networks found: "), String(count_wifiInfo));
	if (count_wifiInfo == 0) {
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(INTL_NO_NETWORKS);
		page_content += FPSTR(BR_TAG);
	} else {
		std::unique_ptr<int[]> indices(new int[count_wifiInfo]);
		debug_outln_info(F("ws: wifi ..."));
		for (unsigned i = 0; i < count_wifiInfo; ++i) {
			indices[i] = i;
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
					indices[j] = -1; // set dup aps to index -1
					++duplicateSsids;
				}
			}
		}

		page_content += FPSTR(INTL_NETWORKS_FOUND);
		page_content += String(count_wifiInfo - duplicateSsids);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(TABLE_TAG_OPEN);
		//if (n > 30) n=30;
		for (int i = 0; i < count_wifiInfo; ++i) {
			if (indices[i] == -1
 #if defined (ESP8266)
				|| wifiInfo[indices[i]].isHidden
 #endif
			) {
				continue;
			}
			// Print SSID and RSSI for each network found
#if defined(CONFIG_IDF_TARGET_ESP32C6)
			page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == WIFI_AUTH_OPEN) ? " " : "🔒"), wifiInfo[indices[i]].RSSI);
#endif
#if defined(CONFIG_IDF_TARGET_ESP32C3)
			page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == WIFI_AUTH_OPEN) ? " " : u8"🔒"), wifiInfo[indices[i]].RSSI);
#endif
		}
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);
		page_content += FPSTR(BR_TAG);
	}
}
