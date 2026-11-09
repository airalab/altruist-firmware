#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../../utils.h"
#include "../../config_manager/config_helpers.h"
#include <SPIFFS.h>

/*****************************************************************
 * Webserver remove config                                       *
 *****************************************************************/
void webserver_removeConfig(String &page_content, bool is_HTTP_GET, bool remove_all) {
	debug_outln_info(F("ws: removeConfig ..."));

	if (is_HTTP_GET) {
		page_content += F("<section class='app-panel app-panel--confirm'>");
		page_content += FPSTR(WEB_REMOVE_CONFIG_CONTENT);
		page_content += F("</section>");

	} else {
		if (remove_all) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
			// Silently remove the desaster backup
			SPIFFS.remove(F("/config.json.old"));
			if (SPIFFS.exists(F("/config.json"))) {	//file exists
				debug_outln_info(F("removing config.json..."));
				if (SPIFFS.remove(F("/config.json"))) {
					page_content += F("<div class='ui-notice ui-notice--ok'><strong>" INTL_CONFIG_DELETED ".</strong></div>");
				} else {
					page_content += F("<div class='ui-notice ui-notice--err'><strong>" INTL_CONFIG_CAN_NOT_BE_DELETED ".</strong></div>");
				}
			} else {
				page_content += F("<div class='ui-notice ui-notice--warn'><strong>" INTL_CONFIG_NOT_FOUND ".</strong></div>");
			}
#pragma GCC diagnostic pop
		} else {
			debug_outln_info(F("Removig WiFi credentials"));
			removeWiFiCredentials();
			page_content += F("<div class='ui-notice ui-notice--ok'><strong>");
			page_content += F(INTL_WIFI_CREDENTIALS_DELETED);
			page_content += F("</strong></div>");
		}
	}
}