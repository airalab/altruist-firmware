#include "robonomics_http_api.h"
#include "../defines.h"
#include "../utils.h"
#include "helpers/message_formatter.h"
#include "../config_manager/config_helpers.h"

void RobonomicsHTTPAPI::setup() {
	api_name = "Robonomics Map";
    _client = new WiFiClient();
    String esp_chipid = get_chipid();
    donated_by = cfg::donated_by;
    rws_owner = cfg::rws_owner;
	current_reg = cfg::current_reg;
	timeout = getConfigUintValue("sending_intervall_ms");
	debug_outln_info(F("Robonomics HTTP API is ready with sending interval (sec): "), String(timeout/1000));
}

void RobonomicsHTTPAPI::_send(JsonDocument &data) {
    int num_of_host;
	String data_to_send;
	formatDataToSend(data_to_send, data);
    debug_outln_info(F("robonomics: "), data_to_send);
	is_ok = false;
    num_of_host = chooseRobonomicsServer(false);
    if (num_of_host == 255) {
        num_of_host = chooseRobonomicsServer(true);
    }
    if (num_of_host != 255) {
        POSTRequest(data_to_send, HOST_ROBONOMICS[num_of_host][0]);
    }
}

void RobonomicsHTTPAPI::formatDataToSend(String &data_to_send, JsonDocument &data) {
	double last_value_GPS_lat;
	double last_value_GPS_lon;
	String datalog_data;
	sscanf(cfg::coords_gps, "%lf,%lf", &last_value_GPS_lat, &last_value_GPS_lon);
	formatRobonomicsString(data, datalog_data);
    String signature;
	addTimeAndSign(datalog_data, signature, robonomics);
    data_to_send = F("{\"robonomics_address\": \"");
    data_to_send += robonomics->getSs58Address();
    data_to_send += "\", \"donated_by\": \"";
    data_to_send += donated_by;
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
}

void RobonomicsHTTPAPI::POSTRequest(const String& data, const char* host) {
	HTTPClient _http;
	String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);
    int result = 0;
    String s_Host(FPSTR(host));
	String s_url(FPSTR(URL_ROBONOMICS));
	debug_outln_info(F("Start POST to "), s_Host);
    _http.setTimeout(20 * 1000);
	_http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid);
    _http.setReuse(false);
    if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url)) {
        _http.addHeader(F("Content-Type"), "application/json");
		_http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
        result = _http.POST(data);
        if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
			debug_outln_info(F("Succeeded - "), s_Host);
			is_ok = true;
		} else if (result >= HTTP_CODE_BAD_REQUEST) {
			debug_outln_info(F("Request failed with error: "), String(result));
			debug_outln_info(F("Details:"), _http.getString());
		} else {
			debug_outln_info(F("Request failed with error: "), String(result));
			debug_outln_info(F("Details:"), _http.getString());
		}
        _http.end();
    } else {
		debug_outln_info(F("Failed connecting to "), s_Host);
	}
}

int RobonomicsHTTPAPI::chooseRobonomicsServer(bool onlyGlobal) {
	HTTPClient _http;
	int num_of_robonomics_host = 255;
	int min_sensors = 255;
	int result = 0;
	String s_url = FPSTR(URL_ROBONOMICS);
	int numRobonomicsHosts = sizeof(HOST_ROBONOMICS) / sizeof(HOST_ROBONOMICS[0]);
	debug_outln_info(F("Number of hosts - "), numRobonomicsHosts);
	debug_outln_info(F("current reg - "), current_reg.c_str());

	for (int i = 0; i < numRobonomicsHosts; i++) {
		if (onlyGlobal) {
			if (strcmp(HOST_ROBONOMICS[i][1], INTL_REGION_GLOBAL) == 0) {
				String s_Host = FPSTR(HOST_ROBONOMICS[i][0]);
			} else {
				continue;
			}
		} else if (strcmp(current_reg.c_str(), HOST_ROBONOMICS[i][1]) == 0) {
			String s_Host = FPSTR(HOST_ROBONOMICS[i][0]);
		} else {
			debug_outln_info(F("Not suit region - "), FPSTR(HOST_ROBONOMICS[i][0]));
			continue;
		}
		String s_Host = FPSTR(HOST_ROBONOMICS[i][0]);
		debug_outln_info(F("Start GET request - "), s_Host);

		if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url)) {
			const char * headerKeys[] = {"sensors-count", "on-server"} ;
			const size_t numberOfHeaders = 2;
			_http.collectHeaders(headerKeys, numberOfHeaders);
			_http.addHeader("Sensor-id", robonomics->getSs58Address());

			result = _http.GET();
			debug_outln_info(F("Result code - "), result);

			if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
				debug_outln_info(F("Succeeded GET request - "), s_Host);
				String header = _http.header("sensors-count");
				String on_server = _http.header("on-server");
				const char *num_of_sensors = header.c_str();
				int num = atoi(num_of_sensors);
				debug_outln_info(F("Amount of sensors - "), num_of_sensors);
				debug_outln_info(F("Sensor on server - "), on_server);
				if (on_server == "True") {
					num_of_robonomics_host = i;
					break;
				}
				if (num < min_sensors) {
					min_sensors = num;
					num_of_robonomics_host = i;
				}
			} else if (result >= HTTP_CODE_BAD_REQUEST) {
				debug_outln_info(F("Request failed with error: "), String(result));
				debug_outln_info(F("Details:"), _http.getString());
			}
			_http.end();

		} else {
			debug_outln_info(F("Failed connecting to "), s_Host);
		}
	}
	if (num_of_robonomics_host < numRobonomicsHosts) {
		debug_outln_info(F("Min sensors host - "), HOST_ROBONOMICS[num_of_robonomics_host][0]);
	} else {
		debug_outln_info(F("No sutable host found"));
	}
	
	return num_of_robonomics_host;
}