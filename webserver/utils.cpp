#include "utils.h"
#include "html-content.h"
#include "../utils.h"
#include "../intl.h"
#include "../defines.h"
#include "../config_manager/config_helpers.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <string.h>

namespace {

String data_row_unit_html(const String& unit) {
	if (unit.length() == 0) {
		return emptyString;
	}
	RESERVE_STRING(s, SMALL_STR);
	s = F("<span class='data-line__unit'>");
	s += unit;
	s += F("</span>");
	return s;
}

void append_data_block_open(String& page_content, const String& label, const char* block_modifier) {
	page_content += F("<div class='data-block");
	if (block_modifier && block_modifier[0] != '\0') {
		page_content += ' ';
		page_content += block_modifier;
	}
	page_content += F("'><h3 class='data-block__title'>");
	page_content += label;
	page_content += F("</h3><div class='data-block__rows'>");
}

} // namespace

void web_page_flush_chunk(String& page_content, WebServer* server) {
	if (!server || page_content.length() == 0) {
		return;
	}
	server->sendContent(page_content);
	page_content = emptyString;
	markMainLoopAlive();
	yield();
}

void web_send_content_progmem(WebServer* server, const char* data, size_t len) {
	if (!server || !data || len == 0) {
		return;
	}
	constexpr size_t kChunk = 512;
	char buf[kChunk];
	for (size_t i = 0; i < len; ) {
		const size_t n = (len - i > kChunk) ? kChunk : (len - i);
		memcpy_P(buf, data + i, n);
		server->sendContent(buf, n);
		i += n;
		markMainLoopAlive();
		yield();
	}
}

void web_page_finish_chunked(WebServer* server) {
	if (!server) {
		return;
	}
	server->sendContent(emptyString);
}

void add_data_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<div class='data-line'><span class='data-line__name'>{p}</span><span class='data-line__reading'><span class='data-line__val'>{v}</span>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	page_content += s;
	page_content += data_row_unit_html(String(unit));
	page_content += F("</span></div>");
}

void add_data_row_from_value(String& page_content, const __FlashStringHelper* param, const __FlashStringHelper* value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<div class='data-line'><span class='data-line__name'>{p}</span><span class='data-line__reading'><span class='data-line__val'>{v}</span>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	page_content += s;
	page_content += data_row_unit_html(String(unit));
	page_content += F("</span></div>");
}

void add_data_row_from_value(String& page_content, const String& param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<div class='data-line'><span class='data-line__name'>{p}</span><span class='data-line__reading'><span class='data-line__val'>{v}</span>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	page_content += s;
	page_content += data_row_unit_html(String(unit));
	page_content += F("</span></div>");
}

void add_data_section_start(String& page_content, const __FlashStringHelper* label, const char* block_modifier) {
	append_data_block_open(page_content, String(label), block_modifier);
}

void add_data_section_start(String& page_content, const String& label, const char* block_modifier) {
	append_data_block_open(page_content, label, block_modifier);
}

void add_data_section_end(String& page_content) {
	page_content += F("</div></div>");
}

void add_reading_metrics_grid_start(String& page_content) {
	page_content += F("<div class='reading-grid'>");
}

void add_reading_metrics_grid_end(String& page_content) {
	page_content += F("</div>");
}

void add_reading_metric_card(String& page_content, const __FlashStringHelper* label, const String& value, const char* unit) {
	page_content += F("<div class='reading-card'><span class='reading-card__label'>");
	page_content += label;
	page_content += F("</span><div class='reading-card__reading'><span class='reading-card__value'>");
	page_content += value;
	page_content += F("</span>");
	if (unit != nullptr && unit[0] != '\0') {
		page_content += F("<span class='reading-card__unit'>");
		page_content += unit;
		page_content += F("</span>");
	}
	page_content += F("</div></div>");
}

void add_reading_metric_card(String& page_content, const String& label, const String& value, const char* unit) {
	page_content += F("<div class='reading-card'><span class='reading-card__label'>");
	page_content += label;
	page_content += F("</span><div class='reading-card__reading'><span class='reading-card__value'>");
	page_content += value;
	page_content += F("</span>");
	if (unit != nullptr && unit[0] != '\0') {
		page_content += F("<span class='reading-card__unit'>");
		page_content += unit;
		page_content += F("</span>");
	}
	page_content += F("</div></div>");
}

void add_data_block_intro(String& page_content, const __FlashStringHelper* intro) {
	page_content += F("<p class='data-block__intro'>");
	page_content += intro;
	page_content += F("</p>");
}

void add_data_api_status_row(String& page_content, const String& api_name, const String& status,
	const String& sends, const String& last_send) {
	page_content += F("<div class='data-api'><span class='data-api__name'>");
	page_content += api_name;
	page_content += F("</span><div class='data-api__details'>");
	if (status == "OK") {
		page_content += F("<span class='data-api__badge data-api__badge--ok'>OK</span>");
	} else {
		page_content += F("<span class='data-api__badge data-api__badge--err'>Error</span>");
	}
	page_content += F("<span class='data-api__detail'><span class='data-api__detail-lbl'>");
	page_content += FPSTR(INTL_API_SENDS_SHORT);
	page_content += F("</span>");
	page_content += sends;
	page_content += F("</span><span class='data-api__detail'><span class='data-api__detail-lbl'>");
	page_content += FPSTR(INTL_API_LAST_SHORT);
	page_content += F("</span>");
	page_content += last_send;
	page_content += F("</span></div></div>");
}

void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const __FlashStringHelper* value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

void add_table_row_from_value(String& page_content, const String& param, const String& value, const char* unit) {
    RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

int32_t calcWiFiSignalQuality(int32_t rssi) {
	// Treat 0 or positive values as 0%
	if (rssi >= 0 || rssi < -100) {
		rssi = -100;
	}
	if (rssi > -50) {
		rssi = -50;
	}
	return (rssi + 100) * 2;
}

void append_wlan_ssid_table_row(String& page_content, const char* ssid, uint8_t encryptionType, int32_t rssi) {
	if (ssid == nullptr) {
		ssid = "";
	}
	page_content += F("<tr><td><a href='#wlanpwd' onclick='setSSID(this)' class='wifi'>");
	page_content += ssid;
	page_content += F("</a>&nbsp;");
	if (encryptionType == WIFI_AUTH_OPEN) {
		page_content += ' ';
	} else {
		// HTML entity — renders as lock icon in the browser.
		page_content += F("&#128274;");
	}
	page_content += F("</td><td style='vertical-align:middle;'>");
	page_content += calcWiFiSignalQuality(rssi);
	page_content += F("%</td></tr>");
}

String add_sensor_type(const String& sensor_text) {
	RESERVE_STRING(s, SMALL_STR);
	s = sensor_text;
	s.replace("{pm}", FPSTR(INTL_PARTICULATE_MATTER));
	s.replace("{t}", FPSTR(INTL_TEMPERATURE));
	s.replace("{h}", FPSTR(INTL_HUMIDITY));
	s.replace("{p}", FPSTR(INTL_PRESSURE));
	s.replace("{l_a}", FPSTR(INTL_LEQ_A));
	return s;
}

String form_checkbox(const ConfigShapeId cfgid, const String& info, const bool linebreak, bool enabled) {
	RESERVE_STRING(s, MED_STR);
	if (enabled) {
		s = F("<div class='form-group'><label for='{n}'>"
		"<input type='checkbox' name='{n}' value='1' id='{n}' {c}/>"
		"<input type='hidden' name='{n}' value='0'/>"
		"{i}</label></div><br/>");
	} else {
		s = F("<div class='form-group'><label for='{n}'>"
		"<input type='checkbox' name='{n}' value='1' id='{n}' {c} disabled/>"
		"<input type='hidden' name='{n}' value='0'/>"
		"{i}</label></div><br/>");
	}

	if (*configShape[cfgid].cfg_val.as_bool) {
		s.replace("{c}", F(" checked='checked'"));
	} else {
		s.replace("{c}", emptyString);
	};
	s.replace("{i}", info);
	s.replace("{n}", String(configShape[cfgid].cfg_key()));
	if (! linebreak) {
		s.replace("<br/>", emptyString);
	}
	return s;
}

String form_submit(const String& value) {
	String s = F(	"<button type='submit' class='submit-btn'>{v}</button>");
	s.replace("{v}", value);
	return s;
}

String form_select_lang() {
	String s_select = F(" selected='selected'");
	String s = F("<div class='form-group'>"
		"<label for='current_lang'>" INTL_LANGUAGE "</label>"
		"<select id='current_lang' name='current_lang'>"
				"<option value='EN'>English (EN)</option>"
				"<option value='RU'>Русский (RU)</option>"
		"</select>"
		"</div>");

	s.replace("'" + String(cfg::current_lang) + "'>", "'" + String(cfg::current_lang) + "'" + s_select + ">");
	return s;
}

String form_select_altruist(JsonDocument& data) {
	String s_select = F(" selected='selected'");
	String s = F("<div class='form-group'>"
				"<label for='chosen_altruist_urban'>Altruist Urban</label>"
				"<div style='display:flex;gap:8px;align-items:center;'>"
				"<select id='chosen_altruist_urban' name='chosen_altruist_urban' style='flex:1;'>");

	JsonArray addresses = data["service_data"]["altruist_addresses"];
	for (JsonVariant v : addresses) {
		const String ip = v.as<String>();

		s += F("<option value='");
		s += ip;
		s += "'";

		if (ip == cfg::chosen_altruist_urban) {
			s += s_select;
		}

		s += ">";
		s += ip;
		s += "</option>";
	}

	s += F("</select>"
		"<button type='button' id='btn_scan_urbans' "
		"style='padding:6px 14px;border:1px solid #ccc;border-radius:4px;background:#f8f8f8;cursor:pointer;white-space:nowrap;'>"
		"&#x1F50D; " INTL_SCAN_BTN "</button></div>"
		"<span id='scan_status' style='font-size:12px;color:#666;'></span>"
		"</div>");
	return s;
}

String form_select_timezone() {
	String s_select = F(" selected='selected'");
	String s = F(
    "<div class='form-group'>"
    "<label for='timezone'>Timezone:</label>"
    "<select id='timezone' name='timezone'>"
        "<option value='<-12>12'>UTC-12</option>"
        "<option value='<-11>11'>UTC-11</option>"
        "<option value='<-10>10'>UTC-10</option>"
        "<option value='<-9>9'>UTC-9</option>"
        "<option value='<-8>8'>UTC-8</option>"
        "<option value='<-7>7'>UTC-7</option>"
        "<option value='<-6>6'>UTC-6</option>"
        "<option value='<-5>5'>UTC-5</option>"
        "<option value='<-4>4'>UTC-4</option>"
        "<option value='<-3>3'>UTC-3</option>"
        "<option value='<-2>2'>UTC-2</option>"
        "<option value='<-1>1'>UTC-1</option>"
        "<option value='<+00>0'>UTC+0</option>"
        "<option value='<+01>-1'>UTC+1</option>"
        "<option value='<+02>-2'>UTC+2</option>"
        "<option value='<+03>-3'>UTC+3</option>"
        "<option value='<+04>-4'>UTC+4</option>"
        "<option value='<+05>-5'>UTC+5</option>"
        "<option value='<+06>-6'>UTC+6</option>"
        "<option value='<+07>-7'>UTC+7</option>"
        "<option value='<+08>-8'>UTC+8</option>"
        "<option value='<+09>-9'>UTC+9</option>"
        "<option value='<+10>-10'>UTC+10</option>"
        "<option value='<+11>-11'>UTC+11</option>"
        "<option value='<+12>-12'>UTC+12</option>"
    "</select>"
    "</div>");

	s.replace("'" + String(cfg::timezone) + "'>", "'" + String(cfg::timezone) + "'" + s_select + ">");
	return s;
}

String form_select_reg() {
	String s_select = F(" selected='selected'");
	String s = F(	"<div class='form-group'>"
					"<label for='current_reg'>" INTL_REGION ":&nbsp;</label>"
					"<select id='current_reg' name='current_reg'>"
					"<option value='" REGION_GLOBAL "'>" INTL_REGION_GLOBAL "</option>"
					"<option value='" REGION_RU "'>" INTL_REGION_RU "</option>"
					"</select>"
					"<p class='form-hint'>" INTL_REGION_HINT "</p>"
					"</div>");

	s.replace("'" + String(cfg::current_reg) + "'>", "'" + String(cfg::current_reg) + "'" + s_select + ">");
	return s;
}

void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length, bool enabled) {
	RESERVE_STRING(s, MED_STR);
	if (enabled) {
		s = F("<div class='form-group'>"
				"<label for='{n}'>{i}</label>"
				"<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'{a}/>"
				"</div>");
	} else {
		s = F("<div class='form-group'>"
			"<label for='{n}'>{i}</label>"
			"<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'{a} disabled/>"
			"</div>");
	}
	String t_value;
	String attrs;
	ConfigShapeEntry c;
	memcpy_P(&c, &configShape[cfgid], sizeof(ConfigShapeEntry));
	switch (c.cfg_type) {
	case Config_Type_UInt:
		t_value = String(*c.cfg_val.as_uint);
		s.replace("{t}", F("number"));
		if (cfgid == Config_leds_brightness) {
			attrs = F(" min='0' max='100' step='1'");
		} else if (cfgid == Config_leds_off_hour || cfgid == Config_leds_on_hour) {
			attrs = F(" min='0' max='23' step='1'");
		} else {
			attrs = F(" min='0' step='1'");
		}
		break;
	case Config_Type_Time:
		t_value = String((*c.cfg_val.as_uint) / 1000);
		s.replace("{t}", F("number"));
		attrs = F(" min='0' step='1'");
		break;
	default:
		if (c.cfg_type == Config_Type_Password) {
			s.replace("{t}", F("password"));
			info = FPSTR(INTL_PASSWORD);
		} else {
			t_value = c.cfg_val.as_str;
			t_value.replace("'", "&#39;");
			s.replace("{t}", F("text"));
		}
	}
	s.replace("{i}", info);
	s.replace("{n}", String(c.cfg_key()));
	s.replace("{v}", t_value);
	s.replace("{l}", String(length));
	s.replace("{a}", attrs);
	page_content += s;
}

void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length) {
    add_form_input(page_content, cfgid, info, length, true);
}

String buildLocalAccessLabel() {
	// Stable UI brand for sidebar / browser tab / footer (not the unique DHCP/mDNS name).
	return F("altruist.local");
}

String buildDeviceAccessHost() {
	String host = String(cfg::local_hostname);
	host.trim();
	if (host.length() == 0) {
		char fallback[32];
		cfg::formatDefaultLocalHostname(fallback, sizeof(fallback), get_chipid().c_str());
		host = fallback;
	}
	if (host.indexOf('.') < 0) {
		host += F(".local");
	}
	return host;
}

String buildGuestDeviceInfoJson(const String& ip, const String& sensor_ss58) {
	DynamicJsonDocument doc(1024);
	doc["format"] = "altruist-device1";
	doc["hostname"] = buildDeviceAccessHost();
	if (ip.length() > 0) {
		doc["ip"] = ip;
	}
	if (sensor_ss58.length() > 0 && strcasecmp(sensor_ss58.c_str(), "Not Set") != 0) {
		doc["sensor"] = sensor_ss58;
	}
	String body;
	serializeJson(doc, body);
	return body;
}

void append_guest_device_access(String& page_content, const String& ip, const String& sensor_ss58) {
	const String info_json = buildGuestDeviceInfoJson(ip, sensor_ss58);

	page_content += F("<div class='guest-access'>"
		"<p class='form-hint guest-access__hint'>");
	page_content += FPSTR(INTL_GUEST_DEVICE_INFO_HINT);
	page_content += F("</p>"
		"<div class='guest-access__row'>"
		"<span class='guest-access__label'>");
	page_content += FPSTR(INTL_GUEST_IP_ADDRESS);
	page_content += F("</span>"
		"<strong class='guest-access__value guest-ip ip-address'>");
	page_content += ip;
	page_content += F("</strong>"
		"<button type='button' class='copy-btn' onclick=\"copyGuestText(this)\" aria-label='Copy'></button>"
		"</div>");
	if (sensor_ss58.length() > 0 && strcasecmp(sensor_ss58.c_str(), "Not Set") != 0) {
		page_content += F("<div class='guest-access__row'>"
			"<span class='guest-access__label'>");
		page_content += FPSTR(INTL_GUEST_SENSOR_ADDRESS);
		page_content += F("</span>"
			"<strong class='guest-access__value guest-access__value--addr'>");
		page_content += sensor_ss58;
		page_content += F("</strong>"
			"<button type='button' class='copy-btn' onclick=\"copyGuestText(this)\" aria-label='Copy'></button>"
			"</div>");
	}
	// Embed JSON on the page so save/share never navigates to /device-info.json
	page_content += F("<script type='application/json' id='guest-device-info-json'>");
	page_content += info_json;
	page_content += F("</script>");

	page_content += F("<button type='button' id='guest-device-info-btn' class='submit-btn guest-device-info__btn'"
		" data-shared='");
	page_content += FPSTR(INTL_GUEST_DEVICE_INFO_SHARED);
	page_content += F("' data-saved='");
	page_content += FPSTR(INTL_GUEST_DEVICE_INFO_SAVED);
	page_content += F("' data-copied='");
	page_content += FPSTR(INTL_GUEST_DEVICE_INFO_COPIED);
	page_content += F("' data-fail='");
	page_content += FPSTR(INTL_GUEST_DEVICE_INFO_SAVE_FAIL);
	page_content += F("'>");
	page_content += FPSTR(INTL_GUEST_DEVICE_INFO_DOWNLOAD);
	page_content += F("</button>"
		"<button type='button' id='guest-device-info-copy' class='encrypt-key-btn encrypt-key-btn--ghost guest-device-info__copy'>");
	page_content += FPSTR(INTL_GUEST_DEVICE_INFO_COPY);
	page_content += F("</button>"
		"<p class='form-hint guest-access__dl-status' id='guest-device-info-status' aria-live='polite'></p>"
		"</div><script>"
		"window.copyGuestText=function(btn){"
		"var row=btn&&btn.closest('.guest-access__row');"
		"var el=row&&row.querySelector('.guest-access__value');"
		"if(!el)return;var t=(el.innerText||el.textContent||'').trim();"
		"function ok(){try{btn.title='OK';}catch(e){}}"
		"if(navigator.clipboard&&navigator.clipboard.writeText){"
		"navigator.clipboard.writeText(t).then(ok).catch(function(){"
		"var o=document.createElement('textarea');o.value=t;document.body.appendChild(o);o.select();"
		"document.execCommand('copy');document.body.removeChild(o);ok();});"
		"}else{var o=document.createElement('textarea');o.value=t;document.body.appendChild(o);o.select();"
		"document.execCommand('copy');document.body.removeChild(o);ok();}"
		"};"
		"(function(){"
		"function infoText(){var el=document.getElementById('guest-device-info-json');return el?(el.textContent||'').trim():'';}"
		"function copyText(t){"
		"if(navigator.clipboard&&navigator.clipboard.writeText)return navigator.clipboard.writeText(t);"
		"return new Promise(function(resolve,reject){try{"
		"var o=document.createElement('textarea');o.value=t;o.setAttribute('readonly','');"
		"o.style.position='fixed';o.style.left='-9999px';document.body.appendChild(o);o.select();"
		"document.execCommand('copy');document.body.removeChild(o);resolve();"
		"}catch(e){reject(e);}});"
		"}"
		"function isIOS(){return /iPad|iPhone|iPod/i.test(navigator.userAgent||'');}"
		"function setStatus(msg){var st=document.getElementById('guest-device-info-status');if(st)st.textContent=msg||'';}"
		"function saveFileDesktop(text){"
		"var b=new Blob([text],{type:'application/octet-stream'});"
		"var u=URL.createObjectURL(b);var a=document.createElement('a');"
		"a.href=u;a.download='altruist-device-info.json';a.rel='noopener';"
		"document.body.appendChild(a);a.click();a.remove();"
		"setTimeout(function(){try{URL.revokeObjectURL(u);}catch(e){}},2500);"
		"}"
		"function shareOrSave(btn){"
		"var text=infoText();if(!text)return Promise.reject(new Error('empty'));"
		"var file=null;try{file=new File([text],'altruist-device-info.json',{type:'application/json'});}catch(e){file=null;}"
		"if(file&&navigator.canShare&&navigator.canShare({files:[file]})){"
		"return navigator.share({files:[file],title:'Altruist device info'})"
		".then(function(){return btn.getAttribute('data-shared')||'';});"
		"}"
		"if(!isIOS()){saveFileDesktop(text);return Promise.resolve(btn.getAttribute('data-saved')||'');}"
		"return copyText(text).then(function(){return btn.getAttribute('data-copied')||'';});"
		"}"
		"var btn=document.getElementById('guest-device-info-btn');"
		"var copyBtn=document.getElementById('guest-device-info-copy');"
		"if(btn){btn.addEventListener('click',function(ev){"
		"ev.preventDefault();ev.stopPropagation();setStatus('…');"
		"shareOrSave(btn).then(setStatus).catch(function(){"
		"copyText(infoText()).then(function(){setStatus(btn.getAttribute('data-copied')||'');})"
		".catch(function(){setStatus(btn.getAttribute('data-fail')||'');});"
		"});"
		"});}"
		"if(copyBtn){copyBtn.addEventListener('click',function(ev){"
		"ev.preventDefault();ev.stopPropagation();"
		"copyText(infoText()).then(function(){"
		"setStatus((btn&&btn.getAttribute('data-copied'))||'');"
		"}).catch(function(){setStatus((btn&&btn.getAttribute('data-fail'))||'');});"
		"});}"
		"})();</script>");
}

void append_guest_success_restart_ui(String& page_content) {
	const unsigned pause_sec = (unsigned)(GUEST_SUCCESS_PAGE_DELAY_MS / 1000UL);
	page_content += F("<p class='form-hint' id='guest-restart-hint'>");
	page_content += FPSTR(INTL_GUEST_RESTART_PAUSE_HINT);
	page_content += F("</p>"
		"<form method='POST' action='/finish_setup' class='guest-finish-form'>"
		"<button type='submit' class='submit-btn guest__setup-finish-btn'>");
	page_content += FPSTR(INTL_GUEST_FINISH_SETUP);
	page_content += F("</button></form>"
		"<script>(function(){var s=");
	page_content += String(pause_sec);
	page_content += F(",el=document.getElementById('guest-restart-hint'),base=");
	page_content += F("'");
	page_content += FPSTR(INTL_GUEST_RESTART_PAUSE_HINT);
	page_content += F("';function tick(){if(!el)return;if(s>0){el.textContent=base+' ('+s+'s)';s--;}else{clearInterval(iv);}}"
		"tick();var iv=setInterval(tick,1000);})();</script>");
}

void append_device_backup_restore_form(String& page_content, const __FlashStringHelper* hint, bool guest_mode) {
	String action = F("/restore-backup");
	if (guest_mode) {
		action = F("/guest-restore");
	}

	page_content += F("<section class='hub-backup__panel hub-backup__panel--restore'>"
		"<h3 class='hub-backup__title'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_RESTORE);
	page_content += F("</h3>"
		"<p class='form-hint'>");
	page_content += hint ? hint : FPSTR(INTL_DEVICE_BACKUP_RESTORE_HINT);
	page_content += F("</p>"
		"<form method='POST' action='");
	page_content += action;
	page_content += F("' enctype='multipart/form-data' class='hub-backup__form' data-backup-restore-form='1'>");
	page_content += F("<div class='hub-backup__file-field'>"
		"<label class='hub-backup__file-label' for='device-backup-file'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_FILE_LABEL);
	page_content += F("</label>");

	if (guest_mode) {
		// Visible native picker — hidden inputs often fail to attach files on iOS Safari.
		page_content += F("<input type='file' class='hub-backup__file-input hub-backup__file-input--visible' "
			"id='device-backup-file' name='backup' accept='.json,application/json,text/json'>");
	} else {
		static uint8_t restore_form_seq = 0;
		const uint8_t form_id = ++restore_form_seq;
		const String file_input_id = String(F("device-backup-file-")) + form_id;
		const String file_name_id = String(F("device-backup-file-name-")) + form_id;

		page_content += F("<div class='hub-backup__file-picker'>"
			"<input type='file' class='hub-backup__file-input' id='");
		page_content += file_input_id;
		page_content += F("' name='backup' accept='.json,application/json'>"
			"<label for='");
		page_content += file_input_id;
		page_content += F("' class='hub-backup__file-btn'>");
		page_content += FPSTR(INTL_DEVICE_BACKUP_FILE_CHOOSE);
		page_content += F("</label>"
			"<span class='hub-backup__file-name' id='");
		page_content += file_name_id;
		page_content += F("' data-empty='");
		{
			String empty = FPSTR(INTL_DEVICE_BACKUP_FILE_EMPTY);
			empty.replace("&", "&amp;");
			empty.replace("\"", "&quot;");
			empty.replace("'", "&#39;");
			page_content += empty;
		}
		page_content += F("'>");
		page_content += FPSTR(INTL_DEVICE_BACKUP_FILE_EMPTY);
		page_content += F("</span></div>"
			"<script>"
			"(function(){"
			"var fileInput=document.getElementById('");
		page_content += file_input_id;
		page_content += F("');"
			"var fileName=document.getElementById('");
		page_content += file_name_id;
		page_content += F("');"
			"if(fileInput&&fileName){"
			"fileInput.addEventListener('change',function(){"
			"var f=fileInput.files&&fileInput.files[0];"
			"fileName.textContent=f?f.name:(fileName.getAttribute('data-empty')||'');"
			"});}"
			"})();"
			"</script>");
	}

	page_content += F("</div>"
		"<div class='confirm-action__buttons'>"
		"<button type='submit' class='confirm-btn confirm-btn--danger' data-backup-restore='1'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_RESTORE);
	page_content += F("</button></div></form></section>");

	if (guest_mode) {
		page_content += F("<script>"
			"(function(){"
			"var form=document.querySelector('[data-backup-restore-form]');"
			"if(!form)return;"
			"var msg='");
		{
			String required = FPSTR(INTL_DEVICE_BACKUP_FILE_REQUIRED);
			required.replace("\\", "\\\\");
			required.replace("'", "\\'");
			page_content += required;
		}
		page_content += F("';"
			"form.addEventListener('submit',function(ev){"
			"var fi=form.querySelector('input[type=file]');"
			"if(!fi||!fi.files||!fi.files.length){ev.preventDefault();alert(msg);return;}"
			"var btn=form.querySelector('[data-backup-restore]');"
			"if(btn){btn.disabled=true;btn.classList.add('is-loading');}"
			"});"
			"})();"
			"</script>");
	}
}

void append_app_sidebar(String& page_content) {
	const String local_host = buildLocalAccessLabel();

	// Nest each submenu directly under its category item (not after the whole hub list).
	page_content += F("<aside class='app-sidebar' aria-label='" INTL_NAV_MAIN "'>"
		"<nav class='app-sidebar__nav'>"
		"<div class='app-sidebar__block app-sidebar__hub'>"
		"<a class='app-sidebar__item app-sidebar__item--local' data-tab='local' href='/'>");
	page_content += local_host;
	page_content += F("</a>"
		"<div class='app-sidebar__block app-sidebar__sub app-sidebar__sub--local'>"
		"<span class='app-sidebar__heading'>");
	page_content += FPSTR(INTL_NAV_MONITOR);
	page_content += F("</span>"
		"<a class='app-sidebar__subitem' href='/#readings'>");
	page_content += FPSTR(INTL_NAV_READINGS);
	page_content += F("</a>"
		"<a class='app-sidebar__subitem' href='/#status'>");
	page_content += FPSTR(INTL_NAV_STATUS);
	page_content += F("</a>"
		"<span class='app-sidebar__heading'>");
	page_content += FPSTR(INTL_NAV_SETTINGS);
	page_content += F("</span>"
		"<a class='app-sidebar__subitem' href='/#settings'>");
	page_content += FPSTR(INTL_CONFIGURATION);
	page_content += F("</a>"
		"<a class='app-sidebar__subitem' href='/#ota'>");
	page_content += FPSTR(INTL_OTA_UPDATE);
	page_content += F("</a>");
#ifdef ALTRUIST_INSIGHT
	page_content += F("<span class='app-sidebar__heading'>");
	page_content += FPSTR(INTL_NAV_MAINTENANCE);
	page_content += F("</span>"
		"<a class='app-sidebar__subitem' href='/#screen'>");
	page_content += FPSTR(INTL_SCREEN_MENU);
	page_content += F("</a>");
#endif
	page_content += F("</div>"
		"<a class='app-sidebar__item app-sidebar__item--social' data-tab='social' href='/social'>");
	page_content += FPSTR(INTL_DASH_GROUP_SOCIAL_TITLE);
	page_content += F("</a>"
		"<div class='app-sidebar__block app-sidebar__sub app-sidebar__sub--social'>"
		"<a class='app-sidebar__subitem' href='/social#map-link'>");
	page_content += FPSTR(INTL_ACTIVE_SENSORS_MAP);
	page_content += F("</a>"
		"<span class='app-sidebar__heading'>");
	page_content += FPSTR(INTL_NAV_SETTINGS);
	page_content += F("</span>"
		"<a class='app-sidebar__subitem' href='/social#settings'>");
	page_content += FPSTR(INTL_HUB_DIV_LOCATION);
	page_content += F("</a>"
		"<a class='app-sidebar__subitem' href='/social#group'>");
	page_content += FPSTR(INTL_GROUP_MENU);
	page_content += F("</a></div>"
		"<a class='app-sidebar__item app-sidebar__item--custom' data-tab='custom' href='/custom'>");
	page_content += FPSTR(INTL_DASH_GROUP_CUSTOM_TITLE);
	page_content += F("</a>"
		"<div class='app-sidebar__block app-sidebar__sub app-sidebar__sub--custom'>"
		"<span class='app-sidebar__heading'>");
	page_content += FPSTR(INTL_NAV_SETTINGS);
	page_content += F("</span>"
		"<a class='app-sidebar__subitem' href='/custom#settings'>");
	page_content += FPSTR(INTL_CONFIG_TAB_INTEGRATIONS);
	page_content += F("</a></div>"
		"<a class='app-sidebar__item app-sidebar__item--advanced' data-tab='advanced' href='/advanced'>");
	page_content += FPSTR(INTL_NAV_ADVANCED);
	page_content += F("</a>"
		"<div class='app-sidebar__block app-sidebar__sub app-sidebar__sub--advanced'>"
		"<span class='app-sidebar__heading'>");
	page_content += FPSTR(INTL_NAV_MAINTENANCE);
	page_content += F("</span>"
		"<a class='app-sidebar__subitem' href='/advanced#debug'>");
	page_content += FPSTR(INTL_DEBUG_LEVEL);
	page_content += F("</a>"
		"<a class='app-sidebar__subitem' href='/advanced#backup'>");
	page_content += FPSTR(INTL_DEVICE_BACKUP_TITLE);
	page_content += F("</a>"
		"<a class='app-sidebar__subitem' href='/advanced#restart'>");
	page_content += FPSTR(INTL_RESTART_SENSOR);
	page_content += F("</a>"
		"<a class='app-sidebar__subitem app-sidebar__subitem--danger' href='/advanced#reset'>");
	page_content += FPSTR(INTL_CONFIGURATION_DELETE);
	page_content += F("</a></div></div>"
		"</nav></aside>");
}

void append_app_page_body_start(String& page_content, const __FlashStringHelper* lead) {
	page_content += F("<div class='app-page-body'>");
	if (lead != nullptr) {
		page_content += F("<p class='app-page-lead'>");
		page_content += lead;
		page_content += F("</p>");
	}
}

void append_app_page_body_end(String& page_content) {
	page_content += F("</div>");
}

void append_hub_page_start(String& page_content) {
	page_content += F("<div class='hub-page'>");
}

void append_hub_page_end(String& page_content) {
	page_content += F("</div>");
}

void append_hub_group_start(String& page_content, const __FlashStringHelper* title,
                            const __FlashStringHelper* intro, const char* modifier) {
	page_content += F("<div class='hub-group");
	if (modifier != nullptr && modifier[0] != '\0') {
		page_content += F(" hub-group--");
		page_content += modifier;
	}
	page_content += F("'><div class='hub-group__head'><h2 class='hub-group__title'>");
	page_content += title;
	page_content += F("</h2>");
	if (intro != nullptr) {
		page_content += F("<p class='hub-group__intro'>");
		page_content += intro;
		page_content += F("</p>");
	}
	page_content += F("</div><div class='hub-group__sections'>");
}

void append_hub_group_end(String& page_content) {
	page_content += F("</div></div>");
}

void append_hub_section_start(String& page_content, const __FlashStringHelper* title, const char* section_id) {
	page_content += F("<section class='hub-section'");
	if (section_id != nullptr && section_id[0] != '\0') {
		page_content += F(" id='");
		page_content += section_id;
		page_content += F("'");
	}
	page_content += F("><div class='hub-section__head'><h2 class='hub-section__title'>");
	page_content += title;
	page_content += F("</h2></div><div class='hub-section__body'>");
}

void append_hub_section_end(String& page_content) {
	page_content += F("</div></section>");
}

void append_hub_config_form_start(String& page_content, const char* form_action) {
	page_content += F("<form method='POST' action='");
	page_content += form_action;
	page_content += F("' id='settings' class='config-form hub-config-form'><div class='hub-config-stack'>");
}

void append_hub_config_form_end(String& page_content, bool load_wifi_list) {
	page_content += F("</div><div class='config-form-footer hub-config-footer'>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += F("</div></form>");
	if (load_wifi_list) {
		page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
	}
}