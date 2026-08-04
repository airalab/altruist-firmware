#include "robonomics_http_api.h"
#include "../defines.h"
#include "../utils.h"
#include "helpers/message_formatter.h"
#include "../config_manager/config_helpers.h"
#include <WiFi.h>

namespace
{

	void logConnectivityAttempt(uint32_t seq, const String &region, const String &host)
	{
		Serial.print(F("[CONNECTIVITY] attempt channel=sensors-connectivity seq="));
		Serial.print(seq);
		Serial.print(F(" region="));
		Serial.print(region);
		Serial.print(F(" host="));
		Serial.println(host);
	}

	void logConnectivitySuccess(uint32_t seq, const String &host, int code, const String &region)
	{
		Serial.print(F("[CONNECTIVITY] success channel=sensors-connectivity seq="));
		Serial.print(seq);
		Serial.print(F(" region="));
		Serial.print(region);
		Serial.print(F(" host="));
		Serial.print(host);
		Serial.print(F(" code="));
		Serial.println(code);
	}

	void logConnectivityFailure(uint32_t seq, const String &reason)
	{
		Serial.print(F("[CONNECTIVITY] failed channel=sensors-connectivity seq="));
		Serial.print(seq);
		Serial.print(F(" reason="));
		Serial.println(reason);
	}

	void logConnectivityFailure(uint32_t seq, const String &reason, const String &host, int code, size_t response_len = 0)
	{
		Serial.print(F("[CONNECTIVITY] failed channel=sensors-connectivity seq="));
		Serial.print(seq);
		Serial.print(F(" reason="));
		Serial.print(reason);
		if (host.length() > 0)
		{
			Serial.print(F(" host="));
			Serial.print(host);
		}
		Serial.print(F(" code="));
		Serial.print(code);
		if (response_len > 0)
		{
			Serial.print(F(" response_len="));
			Serial.print(response_len);
		}
		Serial.println();
	}

	void logConnectivityNoServer(uint32_t seq, const String &region, const String &hosts)
	{
		Serial.print(F("[CONNECTIVITY] failed channel=sensors-connectivity seq="));
		Serial.print(seq);
		Serial.print(F(" reason=no_server_available region="));
		Serial.print(region);
		Serial.print(F(" host="));
		Serial.println(hosts.length() > 0 ? hosts : String(F("-")));
	}

	bool isRuRegion(const String &reg)
	{
		return reg == REGION_RU || reg.equalsIgnoreCase(F("Russia")) || reg == F("Россия");
	}

	bool hostRegionMatches(const String &reg, const char *host_region)
	{
		const String tag = FPSTR(host_region);
		if (reg == tag)
		{
			return true;
		}
		// UI label / legacy aliases for RU
		if (tag == REGION_RU && isRuRegion(reg))
		{
			return true;
		}
		return false;
	}

	String builtInHostsForSelection(const String &reg, bool onlyGlobal)
	{
		String hosts;
		const int n = sizeof(HOST_ROBONOMICS) / sizeof(HOST_ROBONOMICS[0]);
		for (int i = 0; i < n; ++i)
		{
			if (onlyGlobal)
			{
				if (!hostRegionMatches(REGION_GLOBAL, HOST_ROBONOMICS[i][1]))
				{
					continue;
				}
			}
			else if (!hostRegionMatches(reg, HOST_ROBONOMICS[i][1]))
			{
				continue;
			}
			if (hosts.length() > 0)
			{
				hosts += ',';
			}
			hosts += FPSTR(HOST_ROBONOMICS[i][0]);
		}
		return hosts;
	}

} // namespace

void RobonomicsHTTPAPI::setup()
{
	api_name = "Robonomics Map";
	_client = new WiFiClient();
	esp_chipid = get_chipid();
	donated_by = cfg::donated_by;
	rws_owner = cfg::rws_owner;
	current_reg = cfg::current_reg;
	connectivity_host_override = String(cfg::robonomics_connectivity_host);
	connectivity_host_override.trim();
	connectivity_hosts_pool = String(cfg::robonomics_connectivity_hosts);
	connectivity_hosts_pool.trim();
	timeout = getConfigUintValue("sending_intervall_ms");
	debug_outln_info(F("Robonomics HTTP API is ready with sending interval (sec): "), String(timeout / 1000));
}

int RobonomicsHTTPAPI::parseHostPool(const String &pool, String *out_hosts, int max_hosts)
{
	if (!out_hosts || max_hosts <= 0)
		return 0;
	int count = 0;
	String s = pool;
	s.replace("\r", "\n");
	s.replace(",", "\n");
	s.replace(";", "\n");
	s.trim();

	int start = 0;
	while (start < (int)s.length() && count < max_hosts)
	{
		int end = s.indexOf('\n', start);
		if (end < 0)
			end = s.length();
		String item = s.substring(start, end);
		item.trim();
		if (item.startsWith("http://"))
			item.remove(0, 7);
		if (item.startsWith("https://"))
			item.remove(0, 8);
		int slash = item.indexOf('/');
		if (slash >= 0)
			item = item.substring(0, slash);
		item.trim();

		if (item.length() > 0 && item != "http" && item != "https")
		{
			bool dup = false;
			for (int i = 0; i < count; i++)
			{
				if (out_hosts[i] == item)
				{
					dup = true;
					break;
				}
			}
			if (!dup)
				out_hosts[count++] = item;
		}
		start = end + 1;
	}
	return count;
}

int RobonomicsHTTPAPI::chooseRobonomicsServerFromPool(const String &pool)
{
	const int kMaxHosts = 8;
	String hosts[kMaxHosts];
	const int host_count = parseHostPool(pool, hosts, kMaxHosts);
	if (host_count <= 0)
		return 255;

	HTTPClient _http;
	int best_idx = 255;
	int min_sensors = 0x7fffffff;
	int result = 0;
	String s_url = FPSTR(URL_ROBONOMICS);

	for (int i = 0; i < host_count; i++)
	{
		if (WiFi.status() != WL_CONNECTED)
			break;
		const String &s_Host = hosts[i];
		_http.setReuse(false);
		if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url))
		{
			const char *headerKeys[] = {"sensors-count", "on-server"};
			const size_t numberOfHeaders = 2;
			_http.collectHeaders(headerKeys, numberOfHeaders);
			_http.addHeader("Sensor-id", robonomics->getSs58Address());

			result = _http.GET();
			if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED)
			{
				String header = _http.header("sensors-count");
				String on_server = _http.header("on-server");
				int num = atoi(header.c_str());
				if (on_server == "True")
				{
					best_idx = i;
					_http.end();
					break;
				}
				if (num < min_sensors)
				{
					min_sensors = num;
					best_idx = i;
				}
			}
			_http.end();
		}
	}

	if (best_idx >= 0 && best_idx < host_count)
	{
		connectivity_host_override = hosts[best_idx];
		return best_idx;
	}
	return 255;
}

void RobonomicsHTTPAPI::_send(JsonDocument &data)
{
	int num_of_host;
	String data_to_send;
	map_send_seq_active = ++map_send_seq;

	// Prefer live config (region may change after setup()).
	current_reg = cfg::current_reg;
	connectivity_host_override = String(cfg::robonomics_connectivity_host);
	connectivity_host_override.trim();
	connectivity_hosts_pool = String(cfg::robonomics_connectivity_hosts);
	connectivity_hosts_pool.trim();

	debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] Send attempt"));
	if (WiFi.status() != WL_CONNECTED)
	{
		logConnectivityFailure(map_send_seq_active, F("wifi_disconnected"));
		debug_outln_verbose(F("[Map] Skipping send: WiFi is disconnected"));
		is_ok = false;
		return;
	}

	// If time isn't synced yet, signing will fail and we'll spam DNS/HTTP retries.
	// Skip Map sends until NTP time becomes available.
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		logConnectivityFailure(map_send_seq_active, F("time_not_synced"));
		debug_outln_verbose(F("[Map] Skipping send: time not synced yet"));
		is_ok = false;
		return;
	}

	if (!formatDataToSend(data_to_send, data))
	{
		logConnectivityFailure(map_send_seq_active, F("encryption_failed"));
		debug_outln_error(F("[Map] Payload encryption failed; send aborted"));
		is_ok = false;
		return;
	}
	debug_outln_verbose(F("[Map] Payload: "), data_to_send);
	is_ok = false;

	// 1) Pinned single host
	if (connectivity_host_override.length() > 0)
	{
		POSTRequest(data_to_send, connectivity_host_override);
		return;
	}

	// 2) Custom pool list
	if (connectivity_hosts_pool.length() > 0)
	{
		const int sel = chooseRobonomicsServerFromPool(connectivity_hosts_pool);
		if (sel != 255 && connectivity_host_override.length() > 0)
		{
			POSTRequest(data_to_send, connectivity_host_override);
			return;
		}
		// fall back to built-in pool if selection failed
	}

	num_of_host = chooseRobonomicsServer(false);
	// Global fallback only when region is not RU — otherwise RU would silently use 1./2.
	if (num_of_host == 255 && !isRuRegion(current_reg))
	{
		debug_outln_verbose(F("[Map] No regional server found, trying global..."));
		num_of_host = chooseRobonomicsServer(true);
	}
	if (num_of_host != 255)
	{
		POSTRequest(data_to_send, String(FPSTR(HOST_ROBONOMICS[num_of_host][0])));
	}
	else
	{
		if (WiFi.status() != WL_CONNECTED)
		{
			logConnectivityFailure(map_send_seq_active, F("wifi_disconnected"));
		}
		else
		{
			logConnectivityNoServer(map_send_seq_active, current_reg,
				builtInHostsForSelection(current_reg, false));
		}
		debug_outln_verbose(F("[Map] FAILED: No server available (all hosts unreachable or returned errors)"));
		debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] selection failed"));
	}
}

bool RobonomicsHTTPAPI::formatDataToSend(String &data_to_send, JsonDocument &data)
{
	double last_value_GPS_lat = 0.0;
	double last_value_GPS_lon = 0.0;
	String datalog_data;
	int parsed = sscanf(cfg::coords_gps, "%lf,%lf", &last_value_GPS_lat, &last_value_GPS_lon);
	if (parsed != 2 || (last_value_GPS_lat == 0.0 && last_value_GPS_lon == 0.0))
	{
		debug_outln_error(F("[Map] WARNING: GPS coordinates missing or invalid, raw value: "));
		debug_outln_verbose(F("[Map] coords_gps = "), String(cfg::coords_gps));
	}
	else
	{
		debug_outln_verbose(F("[Map] GPS lat="), String(last_value_GPS_lat, 6));
		debug_outln_verbose(F("[Map] GPS lon="), String(last_value_GPS_lon, 6));
	}
	if (!formatRobonomicsString(data, datalog_data, F("sensors-connectivity")))
	{
		data_to_send = "";
		return false;
	}
	if (datalog_data.length() == 0)
	{
		debug_outln_error(F("[Map] WARNING: sensor data string is empty (all sharing disabled or no sensor data?)"));
	}
	String signature;
	addTimeAndSign(datalog_data, signature, robonomics);
	if (signature.length() == 0)
	{
		debug_outln_error(F("[Map] WARNING: signature is empty (time not synced or signing failed)"));
	}
	data_to_send = F("{\"robonomics_address\": \"");
	data_to_send += robonomics->getSs58Address();
	data_to_send += "\", \"donated_by\": \"";
	data_to_send += donated_by;
	data_to_send += "\", \"owner\": \"";
	data_to_send += rws_owner;
	data_to_send += "\", \"device_model\": \"";
	data_to_send += DEVICE_MODEL;
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

void RobonomicsHTTPAPI::POSTRequest(const String &data, const String &host)
{
	HTTPClient _http;
	String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);
	int result = 0;
	if (WiFi.status() != WL_CONNECTED)
	{
		logConnectivityFailure(map_send_seq_active, F("wifi_disconnected"));
		debug_outln_verbose(F("[Map] POST skipped: WiFi disconnected"));
		debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] skipped: wifi disconnected"));
		return;
	}
	const String &s_Host = host;
	logConnectivityAttempt(map_send_seq_active, current_reg, s_Host);
	String s_url(FPSTR(URL_ROBONOMICS));
	debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] POST to ") + s_Host + ":" + String(PORT_ROBONOMICS) + s_url);
	_http.setTimeout(20 * 1000);
	_http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid);
	_http.setReuse(false);
	if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url))
	{
		_http.addHeader(F("Content-Type"), "application/json");
		_http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
		result = _http.POST(data);
		if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED)
		{
			logConnectivitySuccess(map_send_seq_active, s_Host, result, current_reg);
			debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] OK, POST succeeded -> ") + s_Host);
			is_ok = true;
		}
		else if (result >= HTTP_CODE_BAD_REQUEST)
		{
			String response_body = _http.getString();
			logConnectivityFailure(map_send_seq_active, F("http_error"), s_Host, result, response_body.length());
			debug_outln_verbose(F("[Map] FAILED: server returned HTTP error"));
			debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] HTTP code: ") + String(result));
			debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] Response body: ") + response_body);
		}
		else
		{
			logConnectivityFailure(map_send_seq_active, F("http_connection_error"), s_Host, result);
			debug_outln_verbose(F("[Map] FAILED: HTTP error (connection/timeout)"));
			debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] Error code: ") + String(result));
			debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] Details: ") + HTTPClient::errorToString(result));
		}
		_http.end();
	}
	else
	{
		logConnectivityFailure(map_send_seq_active, F("http_begin_failed"), s_Host, result);
		debug_outln_verbose(F("[Map] FAILED: could not begin HTTP connection"));
		debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] Host: ") + s_Host);
	}
}

int RobonomicsHTTPAPI::chooseRobonomicsServer(bool onlyGlobal)
{
	HTTPClient _http;
	int num_of_robonomics_host = 255;
	int min_sensors = 255;
	int result = 0;
	String s_url = FPSTR(URL_ROBONOMICS);
	int numRobonomicsHosts = sizeof(HOST_ROBONOMICS) / sizeof(HOST_ROBONOMICS[0]);
	debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] selecting server: ") + String(numRobonomicsHosts) + " hosts, region=" + current_reg.c_str() + (onlyGlobal ? " (global only)" : ""));

	for (int i = 0; i < numRobonomicsHosts; i++)
	{
		if (WiFi.status() != WL_CONNECTED)
		{
			debug_outln_verbose(F("[Map] Stop server selection: WiFi disconnected"));
			break;
		}
		if (onlyGlobal)
		{
			if (!hostRegionMatches(REGION_GLOBAL, HOST_ROBONOMICS[i][1]))
			{
				continue;
			}
		}
		else if (!hostRegionMatches(current_reg, HOST_ROBONOMICS[i][1]))
		{
			continue;
		}
		String s_Host = FPSTR(HOST_ROBONOMICS[i][0]);
		debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] trying GET ") + s_Host + ":" + String(PORT_ROBONOMICS));

		// Must not reuse TCP across different hosts — end() can keep the socket open
		// ("tcp keep open for reuse") and the next begin() then talks to the wrong server.
		_http.setReuse(false);
		if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url))
		{
			const char *headerKeys[] = {"sensors-count", "on-server"};
			const size_t numberOfHeaders = 2;
			_http.collectHeaders(headerKeys, numberOfHeaders);
			_http.addHeader("Sensor-id", robonomics->getSs58Address());

			result = _http.GET();

			if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED)
			{
				String header = _http.header("sensors-count");
				String on_server = _http.header("on-server");
				int num = atoi(header.c_str());
				debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] OK from ") + s_Host + " sensors=" + header + " on_server=" + on_server);
				if (on_server == "True")
				{
					num_of_robonomics_host = i;
					_http.end();
					break;
				}
				if (num < min_sensors)
				{
					min_sensors = num;
					num_of_robonomics_host = i;
				}
			}
			else if (result >= HTTP_CODE_BAD_REQUEST)
			{
				debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] server error from ") + s_Host + " HTTP " + String(result));
				debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] response: ") + _http.getString());
			}
			else
			{
				debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] connection error to ") + s_Host + " code=" + String(result) + " " + HTTPClient::errorToString(result));
			}
			_http.end();
		}
		else
		{
			debug_outln_verbose(F("[Map] Cannot connect to host"));
			debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] host: ") + s_Host);
		}
	}
	if (num_of_robonomics_host < numRobonomicsHosts)
	{
		debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] selected server: ") + String(FPSTR(HOST_ROBONOMICS[num_of_robonomics_host][0])));
	}
	else
	{
		// Not a hard failure by itself: caller may retry with global-only mode or proceed with other logic.
		debug_outln_verbose(String(F("[Map#")) + String(map_send_seq_active) + F("] no suitable server found in this selection pass"));
	}

	return num_of_robonomics_host;
}
