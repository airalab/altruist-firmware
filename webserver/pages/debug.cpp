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
		"fetch('/serial',{cache:'no-store'}).then(function(r){return r.status===204?Promise.resolve(''):r.text();})"
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

namespace {
void apply_log_request(WebServer& server) {
	if (server.arg("status") == "1") {
		logSupportStatus();
		return;
	}
	if (server.hasArg("lvl")) {
		const String value = server.arg("lvl");
		if (value.length() == 1 && value[0] >= '0' && value[0] <= '5' && value[0] != '2') {
			unsigned int level = value[0] - '0';
#if !defined(ALTRUIST_BUILD_DEBUG)
			level = min(level, static_cast<unsigned int>(DEBUG_MIN_INFO));
#endif
#if defined(ALTRUIST_BUILD_DEBUG)
			if (level < static_cast<unsigned int>(ALTRUIST_FORCE_LOG_LEVEL)) {
				return;
			}
#endif
			cfg::debug = level;
			if (effectiveRuntimeLogLevel() == 0) {
				while (Debug.popLines().length() > 0) {}
			}
			logSupportEvent(0, F("logging"), F("mode_changed"), effectiveRuntimeLogLevel());
		}
	}
}

void append_log_controls(String& page_content, const char* path) {
	page_content += F("<p>" INTL_SUPPORT_VOLATILE "</p>");
#if !defined(ALTRUIST_BUILD_DEBUG)
	page_content += F("<p>" INTL_SUPPORT_LIMIT "</p>");
#else
#if ALTRUIST_FORCE_LOG_LEVEL > 0
	page_content += F("<p>" INTL_SUPPORT_FORCED "</p>");
#endif
#endif
	page_content += F("<div class='debug-level-grid'>");
	const unsigned int levels[] = {0, 1, 3,
#if defined(ALTRUIST_BUILD_DEBUG)
		4, 5,
#endif
	};
	unsigned int active = effectiveRuntimeLogLevel();
	for (unsigned int level : levels) {
#if defined(ALTRUIST_BUILD_DEBUG)
		if (level < static_cast<unsigned int>(ALTRUIST_FORCE_LOG_LEVEL)) {
			continue;
		}
#endif
		page_content += F("<a class='debug-level-btn' href='");
		page_content += path;
		page_content += F("?lvl=");
		page_content += String(level);
		page_content += F("' aria-pressed='");
		page_content += active == level ? F("true") : F("false");
		page_content += F("'>");
		page_content += level == 0 ? F(INTL_SUPPORT_OFF) : level == 1 ? F(INTL_SUPPORT_ERRORS) :
		                level == 3 ? F(INTL_SUPPORT_EVENTS) : level == 4 ? F(INTL_SUPPORT_DETAIL) : F(INTL_SUPPORT_MAX_DETAIL);
		page_content += F("</a>");
	}
	page_content += F("</div><p><a class='debug-level-btn' href='");
	page_content += path;
	page_content += F("?status=1'>" INTL_SUPPORT_STATUS "</a> " INTL_SUPPORT_STATUS_HINT "</p>");
}
} // namespace

void webserver_debug_level(WebServer& server, String& page_content, bool hub_embed) {
	apply_log_request(server);
	if (!hub_embed) {
		page_content += F("<section class='app-panel'><h3 class='app-panel__title'>");
		page_content += F(INTL_SUPPORT_LOG_TITLE);
		page_content += F("</h3>");
	}
	append_debug_log(page_content);
	append_log_controls(page_content, "/debug");
	if (!hub_embed) {
		page_content += F("</section>");
	}
}

void webserver_debug_log_embed(String& page_content) {
	append_debug_log(page_content);
}

void webserver_debug_hub_section(WebServer& server, String& page_content) {
	apply_log_request(server);
	append_debug_log(page_content);
	append_log_controls(page_content, "/advanced");
}
