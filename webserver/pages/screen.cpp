#ifdef ALTRUIST_INSIGHT

#include "pages.h"
#include "../../config_manager/config_defaults.h"
#include "../../config_manager/config_helpers.h"
#include "../../defines.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../utils.h"
#include "../../display/display_manager.h"

extern DisplayManager displayManager;

static void appendModeRadio(String& page, unsigned mode, const __FlashStringHelper* label,
                            const __FlashStringHelper* hint, unsigned selected) {
	page += F("<label style='display:block;margin:12px 0;padding:12px;border:1px solid #e0e0e0;"
		"border-radius:6px;'>"
		"<input type='radio' name='epd_refresh_mode' value='");
	page += String(mode);
	page += F("'");
	if (mode == selected) {
		page += F(" checked");
	}
	page += F("/> <strong>");
	page += label;
	page += F("</strong>"
		"<p style='margin:8px 0 0 22px;font-size:12px;color:#666;'>");
	page += hint;
	page += F("</p></label>");
}

static void appendSaveFeedback(String& page, ScreenSaveResult save_result) {
	if (save_result == ScreenSave_None) {
		return;
	}
	if (save_result == ScreenSave_Ok) {
		page += F("<div style='margin:12px 0;padding:12px;background:#eef6ee;border:1px solid #c8e6c9;"
			"border-radius:6px;color:#2e7d32;'><strong>");
		page += FPSTR(INTL_SCREEN_SAVE_OK);
		page += F("</strong></div>");
		return;
	}
	const __FlashStringHelper* message = FPSTR(INTL_SCREEN_SAVE_FAILED);
	if (save_result == ScreenSave_InvalidMode) {
		message = FPSTR(INTL_SCREEN_SAVE_INVALID_MODE);
	} else if (save_result == ScreenSave_ConfigFailed) {
		message = FPSTR(INTL_SCREEN_SAVE_CONFIG_FAILED);
	}
	page += F("<div style='margin:12px 0;padding:12px;background:#ffebee;border:1px solid #ef9a9a;"
		"border-radius:6px;color:#c62828;'><strong>");
	page += message;
	page += F("</strong></div>");
}

void webserver_screen_page(String& page_content, ScreenSaveResult save_result) {
	const unsigned mode = cfg::epd_refresh_mode;

	page_content += F("<p style='font-size:13px;color:#666;'>");
	page_content += FPSTR(INTL_SCREEN_INTRO);
	page_content += F("</p>");

	appendSaveFeedback(page_content, save_result);

	page_content += F("<form method='POST' action='/screen'>");

	appendModeRadio(page_content, EPD_REFRESH_SAFE, FPSTR(INTL_SCREEN_MODE_SAFE),
	                FPSTR(INTL_SCREEN_MODE_SAFE_HINT), mode);
	appendModeRadio(page_content, EPD_REFRESH_EXPERIMENTAL_PARTIAL, FPSTR(INTL_SCREEN_MODE_EXPERIMENTAL),
	                FPSTR(INTL_SCREEN_MODE_EXPERIMENTAL_HINT), mode);

	page_content += F("<input type='hidden' name='save_screen' value='1'/>");
	page_content += form_submit(FPSTR(INTL_SAVE));
	page_content += F("</form>");
}

ScreenSaveResult webserver_screen_post(WebServer& server) {
	if (!server.hasArg(F("save_screen"))) {
		return ScreenSave_None;
	}

	unsigned mode = cfg::epd_refresh_mode;
	if (server.hasArg(F("epd_refresh_mode"))) {
		mode = static_cast<unsigned>(server.arg(F("epd_refresh_mode")).toInt());
	}
	if (mode > EPD_REFRESH_EXPERIMENTAL_PARTIAL) {
		return ScreenSave_InvalidMode;
	}

	const unsigned old_mode = cfg::epd_refresh_mode;
	if (old_mode == mode) {
		return ScreenSave_Ok;
	}

	cfg::epd_refresh_mode = mode;
	if (!writeConfig()) {
		cfg::epd_refresh_mode = old_mode;
		debug_outln_error(F("[EPD] failed to save screen mode"));
		return ScreenSave_ConfigFailed;
	}

	displayManager.requestEpdFullRefresh();
	debug_outln_info(String(F("[EPD] screen mode set to ")) + String(mode));
	return ScreenSave_Ok;
}

#endif
