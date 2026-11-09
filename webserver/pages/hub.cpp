#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../../config_manager/config_helpers.h"
#include "../../utils.h"
#include "../utils.h"
#include "../../defines.h"
#include <Robonomics.h>

namespace {

void append_hub_restart_section(String& page_content) {
	page_content += F("<div class='confirm-action'>"
		"<p class='confirm-action__question'>");
	page_content += FPSTR(INTL_REALLY_RESTART_SENSOR);
	page_content += F("</p>"
		"<form method='POST' action='/restart' class='confirm-action__form'>"
		"<div class='confirm-action__buttons'>"
		"<button type='submit' class='confirm-btn confirm-btn--danger' name='submit'>");
	page_content += FPSTR(INTL_RESTART);
	page_content += F("</button></div></form></div>");
}

void append_hub_remove_config_section(String& page_content) {
	page_content += F("<div class='confirm-action'>"
		"<form method='POST' action='/removeConfig' class='confirm-action__form js-delete-config'>"
		"<div class='confirm-action__options radio-list'>"
		"<label class='guest-option' for='hub_allConfig'>"
		"<input type='radio' id='hub_allConfig' name='configType' value='all' checked>"
		"<span><strong>");
	page_content += FPSTR(INTL_DELETE_CONFIG_ALL);
	page_content += F("</strong><span class='dash-row__desc'>");
	page_content += FPSTR(INTL_DELETE_CONFIG_ALL_DESC);
	page_content += F("</span></span></label>"
		"<label class='guest-option' for='hub_wifiConfig'>"
		"<input type='radio' id='hub_wifiConfig' name='configType' value='wifi'>"
		"<span><strong>");
	page_content += FPSTR(INTL_DELETE_CONFIG_WIFI);
	page_content += F("</strong><span class='dash-row__desc'>");
	page_content += FPSTR(INTL_DELETE_CONFIG_WIFI_DESC);
	page_content += F("</span></span></label>"
		"</div>"
		"<div class='confirm-action__step' data-delete-step='ask'>"
		"<div class='confirm-action__buttons'>"
		"<button type='button' class='confirm-btn confirm-btn--danger js-delete-ask'>");
	page_content += FPSTR(INTL_DELETE);
	page_content += F("</button></div></div>"
		"<div class='confirm-action__step' data-delete-step='confirm' hidden>"
		"<div class='ui-notice ui-notice--err' role='alert'>"
		"<strong>");
	page_content += FPSTR(INTL_CONFIGURATION_REALLY_DELETE);
	page_content += F("</strong>"
		"<p class='dash-row__desc'>");
	page_content += FPSTR(INTL_CONFIGURATION_DELETE_WARNING);
	page_content += F("</p></div>"
		"<div class='confirm-action__buttons'>"
		"<button type='submit' class='confirm-btn confirm-btn--danger' name='submit'>");
	page_content += FPSTR(INTL_CONFIGURATION_DELETE_CONFIRM);
	page_content += F("</button>"
		"<button type='button' class='confirm-btn confirm-btn--cancel js-delete-cancel'>");
		page_content += FPSTR(INTL_CANCEL);
	page_content += F("</button></div></div></form></div>");
}

void append_hub_backup_section(String& page_content) {
	page_content += F("<div class='hub-backup'>"
		"<section class='hub-backup__panel'>"
		"<h3 class='hub-backup__title'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_DOWNLOAD);
	page_content += F("</h3>"
		"<p class='form-hint'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_HINT);
	page_content += F("</p>"
		"<button type='button' class='encrypt-key-btn encrypt-key-btn--ghost' id='device-backup-download'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_DOWNLOAD);
	page_content += F("</button>"
		"</section>");
	append_device_backup_restore_form(page_content);
	page_content += F("</div>"
		"<script>"
		"(function(){"
		"var btn=document.getElementById('device-backup-download');"
		"if(btn){btn.addEventListener('click',function(){"
		"var a=document.createElement('a');"
		"a.href='/backup.json';"
		"a.setAttribute('download','altruist-backup.json');"
		"document.body.appendChild(a);a.click();document.body.removeChild(a);"
		"});}"
		"})();"
		"</script>");
}

} // namespace

void webserver_append_ota_section(String& page_content, device_status_t& deviceStatus, const char* form_action) {
	if (!form_action || !form_action[0]) {
		form_action = "/";
	}

	page_content += F("<div class='hub-ota'>"
		"<div class='data-sheet'>"
		"<div class='data-block'><div class='data-block__rows'>");
	add_data_row_from_value(page_content, FPSTR(INTL_OTA_CURRENT_VERSION), String(SOFTWARE_VERSION_STR));
	add_data_row_from_value(page_content, "Firmware channel", ALTRUIST_BUILD_CHANNEL);
	page_content += F("<div class='data-line'><span class='data-line__name'>");
	page_content += FPSTR(INTL_LAST_OTA);
	page_content += F("</span><span class='data-line__reading'><span class='data-line__val' id='ota-last-check'>");
	page_content += delayToString(millis() - deviceStatus.last_update_attempt);
	page_content += F("</span></span></div>"
		"<div class='data-line' id='ota-latest-row' hidden>"
		"<span class='data-line__name'>");
	page_content += FPSTR(INTL_OTA_LATEST_VERSION);
	page_content += F("</span><span class='data-line__reading'><span class='data-line__val' id='ota-latest-val'></span>"
		"</span></div></div></div></div>"
		"<div class='hub-ota__actions'>");

	page_content += F("<section class='config-section hub-ota__check'>"
		"<div class='hub-ota__check-row'>"
		"<h2 class='config-section__title'>");
	page_content += FPSTR(INTL_OTA_CHECK_UPDATE);
	page_content += F("</h2>"
		"<button type='button' class='submit-btn' id='ota-check-btn' data-fail='");
	page_content += FPSTR(INTL_OTA_CHECK_FAILED);
	page_content += F("'>");
	page_content += FPSTR(INTL_OTA_CHECK_UPDATE);
	page_content += F("</button></div>"
		"<div class='hub-ota__check-result' id='ota-check-result' hidden>"
		"<div class='hub-ota__check-result-main'>"
		"<p class='hub-ota__check-msg' id='ota-check-msg'></p>"
		"<div class='hub-ota__progress' id='ota-progress' hidden>"
		"<div class='hub-ota__progress-track'><div class='hub-ota__progress-bar' id='ota-progress-bar'></div></div>"
		"<span class='hub-ota__progress-pct' id='ota-progress-pct'>0%</span>"
		"</div></div>"
		"<button type='button' class='submit-btn hub-ota__install-btn' id='ota-install-btn' hidden>");
	page_content += FPSTR(INTL_OTA_INSTALL);
	page_content += F("</button></div></section>");

	page_content += F("<section class='config-section'><h2 class='config-section__title'>");
	page_content += FPSTR(INTL_OTA_SWITCH_LANG);
	page_content += F("</h2><div class='config-section__body'>"
		"<p class='form-hint'><strong>");
	page_content += FPSTR(INTL_OTA_CURRENT_LANG);
	page_content += F(":</strong> ");
	page_content += String(CURRENT_LANG);
	page_content += F("</p>"
		"<form method='POST' action='");
	page_content += form_action;
	page_content += F("'>"
		"<input type='hidden' name='action' value='switch_lang'>");
	page_content += form_select_lang();
	page_content += F("<p class='form-hint'>");
	page_content += FPSTR(INTL_OTA_SWITCH_LANG_NOTE);
	page_content += F("</p>");
	page_content += form_submit(FPSTR(INTL_OTA_SWITCH_LANG));
	page_content += F("</form></div></section>"
		"</div></div>"
		"<script>(function(){"
		"var root=document.querySelector('.hub-ota');"
		"if(!root)return;"
		"var checkBtn=root.querySelector('#ota-check-btn');"
		"var installBtn=root.querySelector('#ota-install-btn');"
		"var result=root.querySelector('#ota-check-result');"
		"var msg=root.querySelector('#ota-check-msg');"
		"var latestRow=root.querySelector('#ota-latest-row');"
		"var latestVal=root.querySelector('#ota-latest-val');"
		"var lastCheck=root.querySelector('#ota-last-check');"
		"var progress=root.querySelector('#ota-progress');"
		"var progressBar=root.querySelector('#ota-progress-bar');"
		"var progressPct=root.querySelector('#ota-progress-pct');"
		"var updatingLabel=\"");
	page_content += FPSTR(INTL_OTA_UPDATING);
	page_content += F("\";"
		"var successLabel=\"");
	page_content += INTL_DISP_OTA_SUCCESS;
	page_content += F("\";"
		"var failLabel=\"");
	page_content += INTL_DISP_OTA_FAILED;
	page_content += F("\";"
		"var restartLabel=\"");
	page_content += INTL_DISP_OTA_RESTARTING;
	page_content += F("\";"
		"var pollTimer=null;"
		"if(!checkBtn||!result||!msg)return;"
		"function failMsg(){return checkBtn.getAttribute('data-fail')||'Error';}"
		"function setProgress(p){"
		"p=Math.max(0,Math.min(100,p|0));"
		"if(progress)progress.hidden=false;"
		"if(progressBar)progressBar.style.width=p+'%';"
		"if(progressPct)progressPct.textContent=p+'%';"
		"msg.textContent=updatingLabel+' - '+p+'%';"
		"}"
		"function stopPoll(){if(pollTimer){clearInterval(pollTimer);pollTimer=null;}}"
		"function startPoll(){"
		"stopPoll();"
		"checkBtn.disabled=true;"
		"if(installBtn)installBtn.hidden=true;"
		"setProgress(0);"
		"pollTimer=setInterval(function(){"
		"fetch('/ota-progress.json',{credentials:'same-origin'}).then(function(r){"
		"if(!r.ok)throw new Error('http');return r.json();"
		"}).then(function(d){"
		"if(d.success){"
		"stopPoll();setProgress(100);"
		"msg.textContent=successLabel+' - '+restartLabel;"
		"msg.className='hub-ota__check-msg';"
		"return;"
		"}"
		"if(d.failed){"
		"stopPoll();"
		"if(progress)progress.hidden=true;"
		"msg.textContent=failLabel;"
		"msg.className='hub-ota__check-msg hub-ota__check-msg--warn';"
		"checkBtn.disabled=false;"
		"return;"
		"}"
		"if(d.in_progress||d.queued){setProgress(d.percent||0);}"
		"}).catch(function(){"
		"stopPoll();"
		"msg.textContent=restartLabel;"
		"msg.className='hub-ota__check-msg';"
		"if(progressBar)progressBar.style.width='100%';"
		"if(progressPct)progressPct.textContent='100%';"
		"});"
		"},800);"
		"}"
		"function showResult(d){"
		"d=d||{};"
		"result.hidden=false;"
		"msg.textContent=d.message||'';"
		"msg.className='hub-ota__check-msg'+(d.status==='failed'?' hub-ota__check-msg--warn':'');"
		"if(installBtn){installBtn.hidden=!d.show_install;installBtn.disabled=false;}"
		"if(lastCheck){lastCheck.textContent=d.last_check||'0s';}"
		"if(latestRow&&latestVal){"
		"if(d.status==='failed'){latestRow.hidden=true;}"
		"else if(d.latest){latestVal.textContent=d.latest;latestRow.hidden=false;}"
		"else{latestRow.hidden=true;}"
		"}"
		"if(d.status==='install'){if(d.updating)updatingLabel=d.updating;startPoll();}"
		"}"
		"checkBtn.addEventListener('click',function(){"
		"checkBtn.disabled=true;"
		"fetch('/ota-check.json',{credentials:'same-origin'}).then(function(r){"
		"if(!r.ok)throw new Error('http');return r.json();"
		"}).then(showResult)"
		".catch(function(){showResult({status:'failed',message:failMsg(),show_install:false});})"
		".finally(function(){checkBtn.disabled=false;});"
		"});"
		"if(installBtn){installBtn.addEventListener('click',function(){"
		"installBtn.disabled=true;"
		"fetch('/ota-install.json',{method:'POST',credentials:'same-origin'}).then(function(r){"
		"if(!r.ok)throw new Error('http');return r.json();"
		"}).then(showResult)"
		".catch(function(){showResult({status:'failed',message:failMsg(),show_install:false});installBtn.disabled=false;});"
		"});}"
		"})();</script>");
}

namespace {

void append_hub_social_map_info(String& page_content, const String& robonomics_address) {
#if !defined(ALTRUIST_URBAN_C3_LITE)
	const char* sensor_ss58 = (robonomics_address.length() > 0) ? robonomics_address.c_str() : nullptr;
	const String map_url = buildSensorsSocialMapUrl(sensor_ss58);
	page_content += F("<div class='hub-map-link' id='map-link'>"
		"<a class='b b-secondary' href='");
	page_content += map_url;
	page_content += F("' target='_blank' rel='noreferrer'>");
	page_content += FPSTR(INTL_ACTIVE_SENSORS_MAP);
	page_content += F("</a></div>");
#else
	(void)page_content;
	(void)robonomics_address;
#endif
}

void append_hub_settings_cards(WebServer& server, String& page_content, bool wificonfig_loop,
                               JsonDocument& data, const char* form_action, const uint16_t* order, size_t order_len,
                               const char* sensor_ss58 = nullptr) {
	append_hub_config_form_start(page_content, form_action);
	for (size_t i = 0; i < order_len; ++i) {
		webserver_config_send_body_get(server, page_content, wificonfig_loop, data, nullptr, order[i], sensor_ss58);
	}
	append_hub_config_form_end(page_content, wificonfig_loop && form_action != nullptr && form_action[0] == '/' && form_action[1] == '\0');
}

} // namespace

void webserver_hub_local(String& page_content, JsonDocument& data, device_status_t& deviceStatus, WebServer& server,
                         bool wificonfig_loop, bool readings_busy) {
	// Settings: WiFi (home + setup AP) → secure → device prefs → system → OTA below
	static const uint16_t kLocalSettingsOrder[] = {
		HubSec_WiFi,
		HubSec_WiFiConfig,
		HubSec_Auth,
		HubSec_LEDs,
		HubSec_Sleep,
		HubSec_Firmware,
	};

	append_hub_page_start(page_content);

	// Urban (no display): readings first so you see sensor data immediately
	append_hub_group_start(page_content, FPSTR(INTL_NAV_MONITOR), nullptr, "monitor");
	append_hub_section_start(page_content, FPSTR(INTL_NAV_READINGS), "readings");
	if (readings_busy || data.overflowed()) {
		page_content += F("<p class='data-busy-msg'>");
		page_content += FPSTR(INTL_DATA_BUSY);
		page_content += F("</p>");
	} else {
		webserver_values(data, page_content, server, true);
		web_page_flush_chunk(page_content, &server);
	}
	append_hub_section_end(page_content);

	append_hub_section_start(page_content, FPSTR(INTL_NAV_STATUS), "status");
	webserver_status_part1(page_content, deviceStatus, server, true);
	web_page_flush_chunk(page_content, &server);
	webserver_status_part2(page_content, deviceStatus, server);
	page_content += F("</div>");
	append_hub_section_end(page_content);
	append_hub_group_end(page_content);

	append_hub_group_start(page_content, FPSTR(INTL_NAV_SETTINGS), nullptr, "settings");
	append_hub_settings_cards(server, page_content, wificonfig_loop, data, "/", kLocalSettingsOrder,
	                         sizeof(kLocalSettingsOrder) / sizeof(kLocalSettingsOrder[0]));
	web_page_flush_chunk(page_content, &server);

	// Check-update / switch-lang after System prefs
	append_hub_section_start(page_content, FPSTR(INTL_OTA_UPDATE), "ota");
	webserver_append_ota_section(page_content, deviceStatus, "/");
	append_hub_section_end(page_content);
	append_hub_group_end(page_content);

#ifdef ALTRUIST_INSIGHT
	append_hub_group_start(page_content, FPSTR(INTL_NAV_MAINTENANCE), nullptr, "maintenance");
	append_hub_section_start(page_content, FPSTR(INTL_SCREEN_MENU), "screen");
	webserver_screen_page(page_content, ScreenSave_None, "/screen", true);
	append_hub_section_end(page_content);
	append_hub_group_end(page_content);
#endif

	append_hub_page_end(page_content);
}

void webserver_hub_social(String& page_content, const String& robonomics_address, Robonomics* robonomics,
                          WebServer& server, JsonDocument& data, RwsGroupApplyResult group_save, bool wificonfig_loop) {
	// Priority: open map → place on map → publish → network → group
	static const uint16_t kSocialSettingsOrder[] = {
		HubSec_GPS,         // 1. location (needed for the map)
		HubSec_DataSharing, // 2. what appears on the map
		HubSec_Robonomics,  // 3. owner / node / connectivity
	};

	append_hub_page_start(page_content);

	page_content += F("<div class='hub-social-top'>");
	append_hub_social_map_info(page_content, robonomics_address);

	const char* sensor_ss58 =
		(robonomics_address.length() > 0) ? robonomics_address.c_str() : nullptr;
	append_hub_settings_cards(server, page_content, wificonfig_loop, data, "/social", kSocialSettingsOrder,
	                         sizeof(kSocialSettingsOrder) / sizeof(kSocialSettingsOrder[0]), sensor_ss58);
	page_content += F("</div>");
	web_page_flush_chunk(page_content, &server);

	append_hub_section_start(page_content, FPSTR(INTL_GROUP_MENU), "group");
	webserver_group_page(page_content, robonomics_address, robonomics, group_save, "/social", true);
	web_page_flush_chunk(page_content, &server);
	append_hub_section_end(page_content);

	append_hub_page_end(page_content);
}

void webserver_hub_custom(String& page_content, WebServer& server) {
	// Priority: most common integrations first
	static const uint16_t kCustomSettingsOrder[] = {
		HubSec_CustomAPI, // 1. Home Assistant / own HTTP API
		HubSec_Influx,    // 2. time-series DB
		HubSec_CSV,       // 3. file export
	};
	StaticJsonDocument<64> empty_data;

	append_hub_page_start(page_content);
	append_hub_settings_cards(server, page_content, false, empty_data, "/custom", kCustomSettingsOrder,
	                         sizeof(kCustomSettingsOrder) / sizeof(kCustomSettingsOrder[0]));
	append_hub_page_end(page_content);
}

void webserver_hub_advanced(String& page_content, WebServer& server, bool wificonfig_loop) {
	StaticJsonDocument<64> empty_data;
	static const uint16_t kAdvancedSettingsOrder[] = {
		HubSec_Debug, // log level + measurement interval
	};

	append_hub_page_start(page_content);

	append_hub_section_start(page_content, FPSTR(INTL_DEBUG_LEVEL), "debug");
	webserver_debug_hub_section(server, page_content);
	append_hub_section_end(page_content);

	append_hub_settings_cards(server, page_content, wificonfig_loop, empty_data, "/advanced", kAdvancedSettingsOrder,
	                         sizeof(kAdvancedSettingsOrder) / sizeof(kAdvancedSettingsOrder[0]));
	web_page_flush_chunk(page_content, &server);

	append_hub_section_start(page_content, FPSTR(INTL_DEVICE_BACKUP_TITLE), "backup");
	append_hub_backup_section(page_content);
	append_hub_section_end(page_content);

	append_hub_section_start(page_content, FPSTR(INTL_RESTART_SENSOR), "restart");
	append_hub_restart_section(page_content);
	append_hub_section_end(page_content);

	append_hub_section_start(page_content, FPSTR(INTL_CONFIGURATION_DELETE), "reset");
	append_hub_remove_config_section(page_content);
	append_hub_section_end(page_content);

	append_hub_page_end(page_content);
}
