#ifdef ALTRUIST_INSIGHT

#include "pages.h"
#include "../../config_manager/config_defaults.h"
#include "../../config_manager/config_helpers.h"
#include "../../defines.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../utils.h"
#include "../../display/display_manager.h"
#include "../../display/screens/desk_screen.h"

extern DisplayManager displayManager;

static void appendModeRadio(String& page, unsigned mode, const __FlashStringHelper* label,
                            const __FlashStringHelper* hint, unsigned selected) {
	page += F("<label class='guest-option'>"
		"<input type='radio' name='epd_refresh_mode' value='");
	page += String(mode);
	page += F("'");
	if (mode == selected) {
		page += F(" checked");
	}
	page += F("/><span><strong>");
	page += label;
	page += F("</strong><span class='dash-row__desc'>");
	page += hint;
	page += F("</span></span></label>");
}

static void appendSaveFeedback(String& page, ScreenSaveResult save_result) {
	if (save_result == ScreenSave_None) {
		return;
	}
	if (save_result == ScreenSave_Ok) {
		page += F("<div class='ui-notice ui-notice--ok'><strong>");
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
	page += F("<div class='ui-notice ui-notice--err'><strong>");
	page += message;
	page += F("</strong></div>");
}

void webserver_screen_page(String& page_content, ScreenSaveResult save_result, const char* form_action, bool hub_embed) {
	const unsigned mode = cfg::epd_refresh_mode;

	if (!hub_embed) {
		append_app_page_body_start(page_content, FPSTR(INTL_SCREEN_INTRO));
	}

	appendSaveFeedback(page_content, save_result);

	page_content += F("<form class='page-form' method='POST' action='");
	page_content += form_action;
	page_content += F("'>");

	appendModeRadio(page_content, EPD_REFRESH_SAFE, FPSTR(INTL_SCREEN_MODE_SAFE),
	                FPSTR(INTL_SCREEN_MODE_SAFE_HINT), mode);
	appendModeRadio(page_content, EPD_REFRESH_EXPERIMENTAL_PARTIAL, FPSTR(INTL_SCREEN_MODE_EXPERIMENTAL),
	                FPSTR(INTL_SCREEN_MODE_EXPERIMENTAL_HINT), mode);

	page_content += F("<div class='form-group'><label for='main_layout'>" INTL_MAIN_LAYOUT "</label>"
		"<select id='main_layout' name='main_layout'>");
	page_content += F("<option value='0'");
	if (cfg::main_layout == MAIN_LAYOUT_DEFAULT) {
		page_content += F(" selected='selected'");
	}
	page_content += F(">" INTL_MAIN_LAYOUT_DEFAULT "</option><option value='1'");
	if (cfg::main_layout == MAIN_LAYOUT_DESK) {
		page_content += F(" selected='selected'");
	}
	page_content += F(">" INTL_MAIN_LAYOUT_DESK "</option></select>"
		"<p class='form-hint'>" INTL_MAIN_LAYOUT_HINT "</p></div>");

	page_content += F("<div id='desk-settings'>");
	page_content += F("<div class='form-group'><label for='desk_interval_s'>" INTL_DESK_INTERVAL "</label>"
		"<input class='input-short' type='number' id='desk_interval_s' name='desk_interval_s' min='15' max='300' step='15' value='");
	page_content += String(cfg::desk_interval_s);
	page_content += F("'/></div>");

	page_content += F("<div class='form-group'><label for='desk_home'>" INTL_DESK_HOME "</label>"
		"<select id='desk_home' name='desk_home'>");
	const char *home_labels[] = {
		INTL_DESK_TITLE_AIR, INTL_DESK_TITLE_CLIMATE_URBAN, INTL_DESK_TITLE_CO2,
		INTL_DESK_TITLE_NOISE, INTL_DESK_TITLE_PRESSURE, INTL_DESK_TITLE_CLIMATE_INSIGHT
	};
	unsigned shown_home = cfg::desk_home;
	if (cfg::standalone && (shown_home == DESK_PAGE_AIR || shown_home == DESK_PAGE_NOISE ||
	                        shown_home == DESK_PAGE_CLIMATE)) {
		shown_home = deskPageIdAt(deskHomeSlot());
	}
	for (unsigned o = 0; o < DESK_PAGE_COUNT; o++) {
		const unsigned i = deskPageIdInOrder(static_cast<uint8_t>(o));
		if (cfg::standalone && (i == DESK_PAGE_AIR || i == DESK_PAGE_NOISE || i == DESK_PAGE_CLIMATE)) {
			continue;
		}
		page_content += F("<option value='");
		page_content += String(i);
		page_content += F("'");
		if (shown_home == i) {
			page_content += F(" selected='selected'");
		}
		page_content += F(">");
		page_content += home_labels[i];
		page_content += F("</option>");
	}
	page_content += F("</select></div>");

	auto deskPageBox = [&](unsigned bit, const char *name, const char *label) {
		if (cfg::standalone && (bit == DESK_PAGE_AIR || bit == DESK_PAGE_NOISE ||
		                        bit == DESK_PAGE_CLIMATE)) {
			return;
		}
		page_content += F("<label class='guest-option'><input type='checkbox' name='");
		page_content += name;
		page_content += F("' value='1'");
		if (cfg::desk_pages & (1u << bit)) {
			page_content += F(" checked");
		}
		page_content += F("/><span>");
		page_content += label;
		page_content += F("</span></label>");
	};
	page_content += F("<p class='form-hint'>" INTL_DESK_PAGES "</p><div class='checkbox-grid'>");
	deskPageBox(DESK_PAGE_AIR, "desk_page_air", INTL_DESK_TITLE_AIR);
	deskPageBox(DESK_PAGE_CLIMATE, "desk_page_climate", INTL_DESK_TITLE_CLIMATE_URBAN);
	deskPageBox(DESK_PAGE_CLIMATE_INSIGHT, "desk_page_climate_insight", INTL_DESK_TITLE_CLIMATE_INSIGHT);
	deskPageBox(DESK_PAGE_CO2, "desk_page_co2", INTL_DESK_TITLE_CO2);
	deskPageBox(DESK_PAGE_NOISE, "desk_page_noise", INTL_DESK_TITLE_NOISE);
	deskPageBox(DESK_PAGE_PRESSURE, "desk_page_pressure", INTL_DESK_TITLE_PRESSURE);
	page_content += F("</div><p class='form-hint'>" INTL_DESK_PAGES_HINT "</p></div>");
	page_content += F("<script>(function(){function t(){var s=document.getElementById('main_layout');"
		"var d=document.getElementById('desk-settings');if(!s||!d)return;"
		"d.style.display=(s.value==='1')?'block':'none';}"
		"var s=document.getElementById('main_layout');if(s){s.addEventListener('change',t);t();}})();</script>");

	page_content += F("<div class='page-form-footer'>"
		"<input type='hidden' name='save_screen' value='1'/>");
	page_content += form_submit(FPSTR(INTL_SAVE));
	page_content += F("</div></form>");

	if (!hub_embed) {
		append_app_page_body_end(page_content);
	}
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

	unsigned layout = cfg::main_layout;
	if (server.hasArg(F("main_layout"))) {
		layout = static_cast<unsigned>(server.arg(F("main_layout")).toInt());
	}
	if (layout > MAIN_LAYOUT_DESK) {
		return ScreenSave_InvalidMode;
	}

	unsigned interval = cfg::desk_interval_s;
	if (server.hasArg(F("desk_interval_s"))) {
		interval = static_cast<unsigned>(server.arg(F("desk_interval_s")).toInt());
	}

	unsigned home = cfg::desk_home;
	if (server.hasArg(F("desk_home"))) {
		home = static_cast<unsigned>(server.arg(F("desk_home")).toInt());
	}

	unsigned pages = 0;
	if (server.hasArg(F("desk_page_air"))) {
		pages |= (1u << DESK_PAGE_AIR);
	}
	if (server.hasArg(F("desk_page_climate")) || server.hasArg(F("desk_page_temp"))) {
		pages |= (1u << DESK_PAGE_CLIMATE);
	}
	if (server.hasArg(F("desk_page_climate_insight")) || server.hasArg(F("desk_page_humidity"))) {
		pages |= (1u << DESK_PAGE_CLIMATE_INSIGHT);
	}
	if (server.hasArg(F("desk_page_co2"))) {
		pages |= (1u << DESK_PAGE_CO2);
	}
	if (server.hasArg(F("desk_page_noise"))) {
		pages |= (1u << DESK_PAGE_NOISE);
	}
	if (server.hasArg(F("desk_page_pressure"))) {
		pages |= (1u << DESK_PAGE_PRESSURE);
	}
	if (cfg::standalone) {
		const unsigned urban = (1u << DESK_PAGE_AIR) | (1u << DESK_PAGE_NOISE) |
		                       (1u << DESK_PAGE_CLIMATE);
		pages = (pages & ~urban) | (cfg::desk_pages & urban);
		if (home == DESK_PAGE_AIR || home == DESK_PAGE_NOISE || home == DESK_PAGE_CLIMATE) {
			home = deskPageIdAt(deskHomeSlot());
		}
	}

	const unsigned old_mode = cfg::epd_refresh_mode;
	const unsigned old_layout = cfg::main_layout;
	const unsigned old_interval = cfg::desk_interval_s;
	const unsigned old_pages = cfg::desk_pages;
	const unsigned old_home = cfg::desk_home;

	cfg::epd_refresh_mode = mode;
	cfg::main_layout = layout;
	cfg::desk_interval_s = interval;
	cfg::desk_pages = pages;
	cfg::desk_home = home;
	deskNormalizeConfig();

	if (old_mode == cfg::epd_refresh_mode && old_layout == cfg::main_layout &&
	    old_interval == cfg::desk_interval_s && old_pages == cfg::desk_pages &&
	    old_home == cfg::desk_home) {
		return ScreenSave_Ok;
	}

	if (!writeConfig()) {
		cfg::epd_refresh_mode = old_mode;
		cfg::main_layout = old_layout;
		cfg::desk_interval_s = old_interval;
		cfg::desk_pages = old_pages;
		cfg::desk_home = old_home;
		debug_outln_error(F("[EPD] failed to save screen mode"));
		return ScreenSave_ConfigFailed;
	}

	displayManager.requestEpdFullRefresh();
	debug_outln_info(String(F("[EPD] screen mode set to ")) + String(cfg::epd_refresh_mode) +
	                 String(F(" layout=")) + String(cfg::main_layout));
	return ScreenSave_Ok;
}

#endif
