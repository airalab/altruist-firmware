#include "utils.h"
#include "html-content.h"
#include "../utils.h"
#include "../intl.h"
#include <ArduinoJson.h>



void add_table_row_from_value(String& page_content, const String& sensor, const String& param, const String& value, const String& unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{s}</td><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{s}", sensor);
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", unit);
	page_content += s;
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

String wlan_ssid_to_table_row(const String& ssid, const String& encryption, int32_t rssi) {
	String s = F(	"<tr>"
					"<td>"
					"<a href='#wlanpwd' onclick='setSSID(this)' class='wifi'>{n}</a>&nbsp;{e}"
					"</td>"
					"<td style='vertical-align:middle;'>"
					"{v}%"
					"</td>"
					"</tr>");
	s.replace("{n}", ssid);
	s.replace("{e}", encryption);
	s.replace("{v}", String(calcWiFiSignalQuality(rssi)));
	return s;
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
	String s = F(	"<button type='submit' class='submit-btn'>Save configuration and restart</button>");
	s.replace("{v}", value);
	return s;
}

String form_select_lang() {
	String s_select = F(" selected='selected'");
	//INTL_LANGUAGE
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
				"<select id='chosen_altruist_urban' name='chosen_altruist_urban'>");

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

	s += F("</select></div>");
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
					"<label for='current_reg'>" INTL_REGION ":&nbsp</label>"
					"<select id='current_reg' name='current_reg'>"
					"<option value='" REGION_GLOBAL "'>" INTL_REGION_GLOBAL "</option>"
					"<option value='" REGION_EU "'>" INTL_REGION_EU "</option>"
					"<option value='" REGION_AS "'>" INTL_REGION_AS "</option>"
					"<option value='" REGION_AF "'>" INTL_REGION_AF "</option>"
					"<option value='" REGION_AU "'>" INTL_REGION_AU "</option>"
					"<option value='" REGION_NA "'>" INTL_REGION_NA "</option>"
					"<option value='" REGION_SA "'>" INTL_REGION_SA "</option>"
					"</select>"
					"</div>");

	s.replace("'" + String(cfg::current_reg) + "'>", "'" + String(cfg::current_reg) + "'" + s_select + ">");
	return s;
}

void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length, bool enabled) {
	RESERVE_STRING(s, MED_STR);
	if (enabled) {
		s = F("<div class='form-group'>"
				"<label for='{n}'>{i}</label>"
				"<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
				"</div>");
	} else {
		s = F("<div class='form-group'>"
			"<label for='{n}'>{i}</label>"
			"<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}' disabled/>"
			"</div>");
	}
	String t_value;
	ConfigShapeEntry c;
	memcpy_P(&c, &configShape[cfgid], sizeof(ConfigShapeEntry));
	switch (c.cfg_type) {
	case Config_Type_UInt:
		t_value = String(*c.cfg_val.as_uint);
		s.replace("{t}", F("number"));
		break;
	case Config_Type_Time:
		t_value = String((*c.cfg_val.as_uint) / 1000);
		s.replace("{t}", F("number"));
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
	page_content += s;
}

void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length) {
    add_form_input(page_content, cfgid, info, length, true);
}