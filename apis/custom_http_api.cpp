#include "custom_http_api.h"
#include "../defines.h"
#include "../utils.h"
#include "helpers/message_formatter.h"
#include "../config_manager/config_helpers.h"
#include <WiFi.h>

void CustomHTTPAPI::setup() {
	api_name = "Custom API";
    _client = new WiFiClient();
    esp_chipid = get_chipid();
    rws_owner = cfg::rws_owner;
    host_custom = cfg::host_custom;
    port_custom = cfg::port_custom;
    url_custom = cfg::url_custom;
	timeout = getConfigUintValue("sending_intervall_ms");
	debug_outln_info(F("Custom HTTP API is ready with sending interval (sec): "), String(timeout/1000));
}

void CustomHTTPAPI::_send(JsonDocument &data) {
	String data_to_send;
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Custom API] Skipping send: WiFi is disconnected"));
		is_ok = false;
		return;
	}
	if (!formatDataToSend(data_to_send, data)) {
		debug_outln_error(F("[Custom API] Payload encryption failed; send aborted"));
		is_ok = false;
		return;
	}
    debug_outln_verbose(F("custom api data: "), data_to_send);
	is_ok = false;
    is_ok = POSTRequest(data_to_send);
}

bool CustomHTTPAPI::formatDataToSend(String &data_to_send, JsonDocument &data) {
	double last_value_GPS_lat;
	double last_value_GPS_lon;
	String datalog_data;
	sscanf(cfg::coords_gps, "%lf,%lf", &last_value_GPS_lat, &last_value_GPS_lon);
	if (!formatRobonomicsString(data, datalog_data, F("custom-http"))) {
		data_to_send = "";
		return false;
	}
    String signature;
	addTimeAndSign(datalog_data, signature, robonomics);
    data_to_send = F("{\"robonomics_address\": \"");
    data_to_send += robonomics->getSs58Address();
    data_to_send += "\", \"owner\": \"";
    data_to_send += rws_owner;
    data_to_send += "\", \"signature\": \"";
    data_to_send += signature;
    data_to_send += "\", \"GPS_lat\": \"";
    data_to_send += String(last_value_GPS_lat, 6);
    data_to_send += "\", \"GPS_lon\": \"";
    data_to_send += String(last_value_GPS_lon, 6);
    data_to_send += "\", \"sensordatavalues\": \"";
    data_to_send += datalog_data;
    data_to_send += "\"}";
	return true;
}

bool CustomHTTPAPI::POSTRequest(const String& data) {
	HTTPClient _http;
	String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);
    int result = 0;
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Custom API] POST skipped: WiFi disconnected"));
		return false;
	}
	debug_outln_info(F("Start POST to "), host_custom);
    _http.setTimeout(20 * 1000);
	_http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid);
    _http.setReuse(false);
    if (_http.begin(*_client, host_custom, port_custom, url_custom)) {
        _http.addHeader(F("Content-Type"), "application/json");
		_http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
        result = _http.POST(data);
        if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
			debug_outln_info(F("Succeeded - "), host_custom);
            _http.end();
			return true;
		} else if (result >= HTTP_CODE_BAD_REQUEST) {
			debug_outln_info(F("Request failed with error: "), String(result));
			debug_outln_verbose(F("Details:"), _http.getString());
		} else {
			debug_outln_info(F("Request failed with error: "), String(result));
			debug_outln_verbose(F("Details:"), HTTPClient::errorToString(result));
		}
        _http.end();
    } else {
		debug_outln_info(F("Failed connecting to "), host_custom);
	}
    return false;
}
