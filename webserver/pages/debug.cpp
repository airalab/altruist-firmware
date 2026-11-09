#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../../utils.h"
#include "../../defines.h"

static void append_debug_log(String &page_content) {
	page_content += F("<pre id='slog' class='debug-log'>");
	page_content += Debug.popLines();
	page_content += F("</pre>"
		"<script>"
		"(function(){"
		"var busy=false;"
		"function slog_update(){"
		"if(busy)return;busy=true;"
		"fetch('/serial').then(function(r){return r.status===204?Promise.resolve(''):r.text();})"
		".then(function(t){"
		"if(!t)return;"
		"var el=document.getElementById('slog');if(!el)return;"
		"var atBottom=(el.scrollTop+el.clientHeight)>=(el.scrollHeight-24);"
		"el.textContent+=t;"
		"if(atBottom)el.scrollTop=el.scrollHeight;"
		"}).catch(function(err){console.log(err);})"
		".then(function(){busy=false;});"
		"}"
		"setInterval(slog_update,2000);"
		"})();"
		"</script>");
}

/*****************************************************************
 * Webserver set debug level                                     *
 *****************************************************************/
void webserver_debug_level(WebServer &server, String &page_content, bool hub_embed) {

	if (server.hasArg("lvl")) {
		debug_outln_info(F("ws: debug level ..."));

		const int lvl = server.arg("lvl").toInt();
		if (lvl >= 0 && lvl <= 5) {
			page_content += F("<div class='ui-notice ui-notice--ok'><strong>");
			page_content += FPSTR(INTL_DEBUG_SETTING_TO);
			page_content += F(" ");

			const __FlashStringHelper* lvlText;
			switch (lvl) {
			case DEBUG_ERROR:
				lvlText = F(INTL_ERROR);
				break;
			case DEBUG_WARNING:
				lvlText = F(INTL_WARNING);
				break;
			case DEBUG_MIN_INFO:
				lvlText = F(INTL_MIN_INFO);
				break;
			case DEBUG_MED_INFO:
				lvlText = F(INTL_MED_INFO);
				break;
			case DEBUG_MAX_INFO:
				lvlText = F(INTL_MAX_INFO);
				break;
			default:
				lvlText = F(INTL_NONE);
			}

			page_content += lvlText;
			page_content += F(".</strong></div>");
		}
	}

	if (!hub_embed) {
		page_content += F("<section class='app-panel'><h3 class='app-panel__title'>");
		page_content += FPSTR(INTL_DEBUG_LEVEL);
		page_content += F("</h3>");
	}
	append_debug_log(page_content);
	if (!hub_embed) {
		page_content += F("</section>");
	}

	if (!hub_embed) {
		page_content += F("<section class='app-panel'><h3 class='app-panel__title'>");
		page_content += FPSTR(INTL_DEBUG_SETTING_TO);
		page_content += F("</h3><div class='debug-level-grid'>"
			"<a class='debug-level-btn' href='/debug?lvl=0'>" INTL_NONE "</a>"
			"<a class='debug-level-btn' href='/debug?lvl=1'>" INTL_ERROR "</a>"
			"<a class='debug-level-btn' href='/debug?lvl=3'>" INTL_MIN_INFO "</a>"
			"<a class='debug-level-btn' href='/debug?lvl=5'>" INTL_MAX_INFO "</a>"
			"</div></section>");
	}
}

void webserver_debug_log_embed(String &page_content) {
	append_debug_log(page_content);
}

void webserver_debug_hub_section(WebServer &server, String &page_content) {
	if (server.hasArg("lvl")) {
		debug_outln_info(F("ws: debug level ..."));

		const int lvl = server.arg("lvl").toInt();
		if (lvl >= 0 && lvl <= 5) {
			page_content += F("<div class='ui-notice ui-notice--ok'><strong>");
			page_content += FPSTR(INTL_DEBUG_SETTING_TO);
			page_content += F(" ");

			const __FlashStringHelper* lvlText;
			switch (lvl) {
			case DEBUG_ERROR:
				lvlText = F(INTL_ERROR);
				break;
			case DEBUG_WARNING:
				lvlText = F(INTL_WARNING);
				break;
			case DEBUG_MIN_INFO:
				lvlText = F(INTL_MIN_INFO);
				break;
			case DEBUG_MED_INFO:
				lvlText = F(INTL_MED_INFO);
				break;
			case DEBUG_MAX_INFO:
				lvlText = F(INTL_MAX_INFO);
				break;
			default:
				lvlText = F(INTL_NONE);
			}

			page_content += lvlText;
			page_content += F(".</strong></div>");
		}
	}

	append_debug_log(page_content);
	page_content += F("<div class='debug-level-grid'>"
		"<a class='debug-level-btn' href='/advanced?lvl=0'>" INTL_NONE "</a>"
		"<a class='debug-level-btn' href='/advanced?lvl=1'>" INTL_ERROR "</a>"
		"<a class='debug-level-btn' href='/advanced?lvl=3'>" INTL_MIN_INFO "</a>"
		"<a class='debug-level-btn' href='/advanced?lvl=5'>" INTL_MAX_INFO "</a>"
		"</div>");
}
