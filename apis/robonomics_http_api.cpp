#include "robonomics_http_api.h"
#include "../defines.h"
#include "../utils.h"
#include "helpers/message_formatter.h"
#include "../config_manager/config_helpers.h"
#include <WiFi.h>
#include <ArduinoJson.h>

void RobonomicsHTTPAPI::setup() {
	api_name = "Robonomics Map";
    _client = new WiFiClient();
    esp_chipid = get_chipid();
    donated_by = cfg::donated_by;
    rws_owner = cfg::rws_owner;
	current_reg = cfg::current_reg;
	timeout = getConfigUintValue("sending_intervall_ms");
	debug_outln_info(F("Robonomics HTTP API is ready with sending interval (sec): "), String(timeout/1000));
}

void RobonomicsHTTPAPI::_send(JsonDocument &data) {
	String data_to_send;
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Map] Skipping send: WiFi is disconnected"));
		is_ok = false;
		return;
	}
	formatDataToSend(data_to_send, data);
	debug_outln_verbose(F("[Map] Payload: "), data_to_send);
	is_ok = false;
	if (chooseRobonomicsServer()) {
		POSTRequest(data_to_send, selectedHost);
	} else {
		debug_outln_error(F("[Map] FAILED: No server available (all hosts unreachable or returned errors)"));
	}
}

void RobonomicsHTTPAPI::formatDataToSend(String &data_to_send, JsonDocument &data) {
	double last_value_GPS_lat = 0.0;
	double last_value_GPS_lon = 0.0;
	String datalog_data;
	int parsed = sscanf(cfg::coords_gps, "%lf,%lf", &last_value_GPS_lat, &last_value_GPS_lon);
	if (parsed != 2 || (last_value_GPS_lat == 0.0 && last_value_GPS_lon == 0.0)) {
		debug_outln_error(F("[Map] WARNING: GPS coordinates missing or invalid, raw value: "));
		debug_outln_verbose(F("[Map] coords_gps = "), String(cfg::coords_gps));
	} else {
		debug_outln_verbose(F("[Map] GPS lat="), String(last_value_GPS_lat, 6));
		debug_outln_verbose(F("[Map] GPS lon="), String(last_value_GPS_lon, 6));
	}
	formatRobonomicsString(data, datalog_data);
	if (datalog_data.length() == 0) {
		debug_outln_error(F("[Map] WARNING: sensor data string is empty (all sharing disabled or no sensor data?)"));
	}
    String signature;
	addTimeAndSign(datalog_data, signature, robonomics);
	if (signature.length() == 0) {
		debug_outln_error(F("[Map] WARNING: signature is empty (time not synced or signing failed)"));
	}
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

void RobonomicsHTTPAPI::POSTRequest(const String& data, const String& host) {
	POSTRequest(data, host.c_str());
}

void RobonomicsHTTPAPI::POSTRequest(const String& data, const char* host) {
	HTTPClient _http;
	String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);
	int result = 0;
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Map] POST skipped: WiFi disconnected"));
		return;
	}
	String s_Host(FPSTR(host));
	String s_url(FPSTR(URL_ROBONOMICS));
	debug_outln_verbose(F("[Map] POST to "), s_Host + ":" + String(PORT_ROBONOMICS) + s_url);
    _http.setTimeout(20 * 1000);
	_http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid);
    _http.setReuse(false);
    if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url)) {
        _http.addHeader(F("Content-Type"), "application/json");
		_http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
        result = _http.POST(data);
        if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
			debug_outln_info(F("[Map] OK, POST succeeded -> "), s_Host);
			is_ok = true;
		} else if (result >= HTTP_CODE_BAD_REQUEST) {
			debug_outln_error(F("[Map] FAILED: server returned HTTP error"));
			debug_outln_verbose(F("[Map] HTTP code: "), String(result));
			debug_outln_verbose(F("[Map] Response body: "), _http.getString());
		} else {
			debug_outln_error(F("[Map] FAILED: HTTP error (connection/timeout)"));
			debug_outln_verbose(F("[Map] Error code: "), String(result));
			debug_outln_verbose(F("[Map] Details: "), HTTPClient::errorToString(result));
		}
        _http.end();
    } else {
		debug_outln_error(F("[Map] FAILED: could not begin HTTP connection"));
		debug_outln_verbose(F("[Map] Host: "), s_Host);
	}
}

bool RobonomicsHTTPAPI::probeServer(HTTPClient& http, const String& host, int& sensors, bool& onServer) {
	String s_url = FPSTR(URL_ROBONOMICS);
	debug_outln_verbose(F("[Map] Trying GET "), host + ":" + String(PORT_ROBONOMICS));

	if (!http.begin(*_client, host, PORT_ROBONOMICS, s_url)) {
		debug_outln_error(F("[Map] Cannot connect to host"));
		debug_outln_verbose(F("[Map] Host: "), host);
		return false;
	}

	const char* headerKeys[] = {"sensors-count", "on-server"};
	http.collectHeaders(headerKeys, 2);
	http.addHeader("Sensor-id", robonomics->getSs58Address());

	int result = http.GET();
	if (result < HTTP_CODE_OK || result > HTTP_CODE_ALREADY_REPORTED) {
		if (result >= HTTP_CODE_BAD_REQUEST) {
			debug_outln_verbose(F("[Map] Server error from "), host + " HTTP " + String(result));
		} else {
			debug_outln_verbose(F("[Map] Connection error to "), host + " code=" + String(result) + " " + HTTPClient::errorToString(result));
		}
		http.end();
		return false;
	}

	String body = http.getString();
	sensors = atoi(http.header("sensors-count").c_str());
	onServer = (http.header("on-server") == "True");
	debug_outln_verbose(F("[Map] OK from "), host + " sensors=" + String(sensors) + " on_server=" + String(onServer));

	if (body.length() > 0 && body[0] == '{') {
		DynamicJsonDocument peersDoc(2048);
		DeserializationError err = deserializeJson(peersDoc, body);
		if (!err && peersDoc.containsKey("servers")) {
			JsonArray serverList = peersDoc["servers"];
			discoveredCount = 0;
			for (JsonObject srv : serverList) {
				if (discoveredCount >= MAX_DISCOVERED_SERVERS) break;
				const char* h = srv["host"];
				if (!h) continue;
				discoveredServers[discoveredCount].host = h;
				discoveredServers[discoveredCount].sensors = srv["sensors"] | 0;
				discoveredServers[discoveredCount].onServer = false;
				discoveredCount++;
			}
			debug_outln_verbose(F("[Map] Discovered peers from JSON: "), String(discoveredCount));
		}
	}

	http.end();
	return true;
}

bool RobonomicsHTTPAPI::discoverServers() {
	if (WiFi.status() != WL_CONNECTED) return false;

	IPAddress resolved;
	if (WiFi.hostByName(DISCOVERY_HOST, resolved)) {
		debug_outln_verbose(F("[Map] DNS resolved "), String(DISCOVERY_HOST) + " -> " + resolved.toString());

		HTTPClient http;
		http.setTimeout(20 * 1000);
		http.setReuse(false);
		int sensors = 0;
		bool onServer = false;

		if (probeServer(http, resolved.toString(), sensors, onServer)) {
			if (onServer) {
				selectedHost = resolved.toString();
				return true;
			}
			if (discoveredCount > 0) return true;
			discoveredServers[0].host = resolved.toString();
			discoveredServers[0].sensors = sensors;
			discoveredServers[0].onServer = false;
			discoveredCount = 1;
			return true;
		}
	} else {
		debug_outln_verbose(F("[Map] DNS resolve failed for "), String(DISCOVERY_HOST));
	}
	return false;
}

bool RobonomicsHTTPAPI::chooseRobonomicsServer() {
	selectedHost = "";
	discoveredCount = 0;

	if (discoverServers() && selectedHost.length() > 0) {
		debug_outln_verbose(F("[Map] Selected server (already registered): "), selectedHost);
		return true;
	}

	if (discoveredCount > 0) {
		HTTPClient http;
		http.setTimeout(20 * 1000);
		http.setReuse(false);
		int bestIdx = -1;
		int minSensors = INT_MAX;

		for (int i = 0; i < discoveredCount; i++) {
			if (WiFi.status() != WL_CONNECTED) break;
			int sensors = 0;
			bool onServer = false;
			if (probeServer(http, discoveredServers[i].host, sensors, onServer)) {
				if (onServer) {
					selectedHost = discoveredServers[i].host;
					debug_outln_verbose(F("[Map] Selected server (registered, discovered): "), selectedHost);
					return true;
				}
				if (sensors < minSensors) {
					minSensors = sensors;
					bestIdx = i;
				}
			}
		}
		if (bestIdx >= 0) {
			selectedHost = discoveredServers[bestIdx].host;
			debug_outln_verbose(F("[Map] Selected server (least loaded, discovered): "), selectedHost);
			return true;
		}
	}

	debug_outln_verbose(F("[Map] Discovery failed, falling back to hardcoded servers"));
	int numHosts = sizeof(HOST_ROBONOMICS) / sizeof(HOST_ROBONOMICS[0]);
	int bestIdx = -1;
	int minSensors = INT_MAX;
	HTTPClient http;
	http.setTimeout(20 * 1000);
	http.setReuse(false);

	for (int i = 0; i < numHosts; i++) {
		if (WiFi.status() != WL_CONNECTED) break;
		String host = FPSTR(HOST_ROBONOMICS[i][0]);
		int sensors = 0;
		bool onServer = false;
		if (probeServer(http, host, sensors, onServer)) {
			if (onServer) {
				selectedHost = host;
				debug_outln_verbose(F("[Map] Selected server (registered, hardcoded): "), selectedHost);
				return true;
			}
			if (sensors < minSensors) {
				minSensors = sensors;
				bestIdx = i;
			}
		}
	}

	if (bestIdx >= 0) {
		selectedHost = FPSTR(HOST_ROBONOMICS[bestIdx][0]);
		debug_outln_verbose(F("[Map] Selected server (least loaded, hardcoded): "), selectedHost);
		return true;
	}

	debug_outln_error(F("[Map] No suitable server found among all hosts"));
	return false;
}