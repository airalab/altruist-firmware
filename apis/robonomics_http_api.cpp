#include "robonomics_http_api.h"
#include "../defines.h"
#include "../utils.h"
#include "helpers/message_formatter.h"
#include "../config_manager/config_helpers.h"
#include <WiFi.h>
#include <ArduinoJson.h>

/**
 * setup — initialize the Robonomics HTTP API.
 *
 * Creates a persistent WiFiClient, reads device identity (chip ID) and
 * user configuration (donated_by, rws_owner, region, send interval).
 * Called once at boot.
 */
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

/**
 * _send — main entry point for sending sensor data to the Robonomics map.
 *
 * Called periodically by the API framework. Formats the sensor payload,
 * selects the best server via dynamic discovery, and POSTs the data.
 *
 * @param data  JSON document containing all sensor readings
 */
void RobonomicsHTTPAPI::_send(JsonDocument &data) {
	String data_to_send;

	// Guard: skip sending entirely if WiFi is down to avoid wasting time on HTTP calls
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Map] Skipping send: WiFi is disconnected"));
		is_ok = false;
		return;
	}

	formatDataToSend(data_to_send, data);
	debug_outln_verbose(F("[Map] Payload: "), data_to_send);

	// Reset success flag — will be set to true only if POST succeeds
	is_ok = false;

	// chooseRobonomicsServer() runs the full discovery + fallback pipeline
	// and stores the result in selectedHost
	if (chooseRobonomicsServer()) {
		POSTRequest(data_to_send, selectedHost);
	} else {
		debug_outln_error(F("[Map] FAILED: No server available (all hosts unreachable or returned errors)"));
	}
}

/**
 * formatDataToSend — build the JSON payload string for the Robonomics map API.
 *
 * Extracts GPS coordinates from config, formats sensor data via Robonomics
 * message formatter, signs the data, and assembles the final JSON string.
 * Uses manual string concatenation (not ArduinoJson serialization) to minimize
 * memory allocation — the payload structure is fixed and simple.
 *
 * @param data_to_send  [out] The assembled JSON string ready for POST
 * @param data          Sensor data JSON document
 */
void RobonomicsHTTPAPI::formatDataToSend(String &data_to_send, JsonDocument &data) {
	double last_value_GPS_lat = 0.0;
	double last_value_GPS_lon = 0.0;
	String datalog_data;

	// Parse GPS from the config string "lat,lon" format
	int parsed = sscanf(cfg::coords_gps, "%lf,%lf", &last_value_GPS_lat, &last_value_GPS_lon);
	if (parsed != 2 || (last_value_GPS_lat == 0.0 && last_value_GPS_lon == 0.0)) {
		debug_outln_error(F("[Map] WARNING: GPS coordinates missing or invalid, raw value: "));
		debug_outln_verbose(F("[Map] coords_gps = "), String(cfg::coords_gps));
	} else {
		debug_outln_verbose(F("[Map] GPS lat="), String(last_value_GPS_lat, 6));
		debug_outln_verbose(F("[Map] GPS lon="), String(last_value_GPS_lon, 6));
	}

	// Format sensor readings into Robonomics datalog string format
	formatRobonomicsString(data, datalog_data);
	if (datalog_data.length() == 0) {
		debug_outln_error(F("[Map] WARNING: sensor data string is empty (all sharing disabled or no sensor data?)"));
	}

	// Sign the datalog payload with the device's Robonomics key
    String signature;
	addTimeAndSign(datalog_data, signature, robonomics);
	if (signature.length() == 0) {
		debug_outln_error(F("[Map] WARNING: signature is empty (time not synced or signing failed)"));
	}

	// Build JSON payload by string concatenation — avoids ArduinoJson overhead
	// for a fixed-structure payload
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

/**
 * POSTRequest (String overload) — convenience wrapper that delegates to the
 * const char* version. Exists because selectedHost is a String but the
 * original API used const char*.
 */
void RobonomicsHTTPAPI::POSTRequest(const String& data, const String& host) {
	POSTRequest(data, host.c_str());
}

/**
 * POSTRequest — send sensor data to a specific connectivity server via HTTP POST.
 *
 * Sends the JSON payload to host:PORT_ROBONOMICS/ with content-type JSON
 * and a custom X-Sensor header for device identification. Handles HTTP response
 * codes and logs success/failure for debugging.
 *
 * @param data  JSON string payload to send
 * @param host  Target server hostname or IP
 */
void RobonomicsHTTPAPI::POSTRequest(const String& data, const char* host) {
	HTTPClient _http;
	String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);
	int result = 0;

	// Re-check WiFi before each POST — connection may drop between discover and send
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Map] POST skipped: WiFi disconnected"));
		return;
	}

	String s_Host(FPSTR(host));
	String s_url(FPSTR(URL_ROBONOMICS));
	debug_outln_verbose(F("[Map] POST to "), s_Host + ":" + String(PORT_ROBONOMICS) + s_url);

	// 20-second timeout — generous for ESP32 over potentially slow WiFi,
	// but prevents indefinite blocking if the server is unreachable
    _http.setTimeout(20 * 1000);
	_http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid);
	// Disable connection reuse — each send cycle may target a different server
    _http.setReuse(false);
    if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url)) {
        _http.addHeader(F("Content-Type"), "application/json");
		// X-Sensor header lets the server identify this device independently of the payload
		_http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
        result = _http.POST(data);

		// 2xx range (200-208) = success
        if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
			debug_outln_info(F("[Map] OK, POST succeeded -> "), s_Host);
			is_ok = true;
		// 4xx/5xx = server-side rejection (bad data, auth error, etc.)
		} else if (result >= HTTP_CODE_BAD_REQUEST) {
			debug_outln_error(F("[Map] FAILED: server returned HTTP error"));
			debug_outln_verbose(F("[Map] HTTP code: "), String(result));
			debug_outln_verbose(F("[Map] Response body: "), _http.getString());
		// Negative codes or other = transport-level failure (timeout, DNS, connection refused)
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

/**
 * probeServer — check a single connectivity server's availability and metadata.
 *
 * Sends a GET request with a "Sensor-id" header so the server can report whether
 * this specific device is already registered. Reads two custom response headers:
 *   - "sensors-count": number of sensors on this server (for load balancing)
 *   - "on-server": "True" if this device is already registered
 *
 * Additionally, parses the response body for a JSON "servers" array — the server
 * may advertise its peers, which we store in discoveredServers[] for later probing.
 *
 * @param http      HTTPClient instance — reused across calls to avoid repeated allocation
 * @param host      IP address or hostname to probe
 * @param sensors   [out] Number of sensors currently on this server
 * @param onServer  [out] Whether this device is already registered on this server
 * @return true if server responded with HTTP 2xx, false otherwise
 */
bool RobonomicsHTTPAPI::probeServer(HTTPClient& http, const String& host, int& sensors, bool& onServer) {
	String s_url = FPSTR(URL_ROBONOMICS);
	debug_outln_verbose(F("[Map] Trying GET "), host + ":" + String(PORT_ROBONOMICS));

	if (!http.begin(*_client, host, PORT_ROBONOMICS, s_url)) {
		debug_outln_error(F("[Map] Cannot connect to host"));
		debug_outln_verbose(F("[Map] Host: "), host);
		return false;
	}

	// Tell the server to include these custom headers in the response —
	// ESP32 HTTPClient strips unknown headers unless explicitly collected
	const char* headerKeys[] = {"sensors-count", "on-server"};
	http.collectHeaders(headerKeys, 2);

	// Send our Robonomics address so the server can check if we're registered
	http.addHeader("Sensor-id", robonomics->getSs58Address());

	int result = http.GET();

	// Any non-2xx response means this server is not usable
	if (result < HTTP_CODE_OK || result > HTTP_CODE_ALREADY_REPORTED) {
		if (result >= HTTP_CODE_BAD_REQUEST) {
			debug_outln_verbose(F("[Map] Server error from "), host + " HTTP " + String(result));
		} else {
			debug_outln_verbose(F("[Map] Connection error to "), host + " code=" + String(result) + " " + HTTPClient::errorToString(result));
		}
		http.end();
		return false;
	}

	// Extract server metadata from custom response headers
	String body = http.getString();
	sensors = atoi(http.header("sensors-count").c_str());
	onServer = (http.header("on-server") == "True");
	debug_outln_verbose(F("[Map] OK from "), host + " sensors=" + String(sensors) + " on_server=" + String(onServer));

	// Parse the response body for a peer list — servers advertise each other.
	// The body may be empty or non-JSON (e.g., plain "OK"), so we check for '{' first.
	if (body.length() > 0 && body[0] == '{') {
		// 2048 bytes is enough for ~10 server entries with host + sensors fields.
		// Each server JSON object is roughly ~80-100 bytes, so 2048 handles up to ~20
		// with headroom for the outer structure. Keeping it small avoids heap fragmentation
		// on ESP32 which has limited contiguous memory.
		DynamicJsonDocument peersDoc(2048);
		DeserializationError err = deserializeJson(peersDoc, body);
		if (!err && peersDoc.containsKey("servers")) {
			JsonArray serverList = peersDoc["servers"];

			// Reset discovered list — each probe refreshes the peer list entirely,
			// because different servers may report different peer sets
			discoveredCount = 0;

			for (JsonObject srv : serverList) {
				// Stop if we've filled our fixed-size buffer
				if (discoveredCount >= MAX_DISCOVERED_SERVERS) break;

				const char* h = srv["host"];
				// Skip malformed entries with missing host field
				if (!h) continue;

				discoveredServers[discoveredCount].host = h;
				// Default sensors to 0 if the field is missing (| 0 is ArduinoJson's default operator)
				discoveredServers[discoveredCount].sensors = srv["sensors"] | 0;
				// We don't know if we're registered on discovered peers yet —
				// that will be determined when we probe each one individually
				discoveredServers[discoveredCount].onServer = false;
				discoveredCount++;
			}
			debug_outln_verbose(F("[Map] Discovered peers from JSON: "), String(discoveredCount));
		}
	}

	http.end();
	return true;
}

/**
 * discoverServers — resolve the discovery hostname and probe it for peer servers.
 *
 * Resolves DISCOVERY_HOST via DNS. DNS may return different IPs over time (round-robin),
 * which is intentional — it distributes discovery load across multiple nodes.
 * Note: WiFi.hostByName() returns only ONE IP (the first DNS A-record), not all of them.
 * This is a limitation of the ESP32 lwIP stack.
 *
 * After resolving, probes the discovery node. Three outcomes are possible:
 *   1. Device is already registered on discovery node → use it directly (selectedHost set)
 *   2. Discovery node returned a peer list → stored in discoveredServers[] for later probing
 *   3. Discovery node is reachable but returned no peers → add it as a single candidate
 *
 * @return true if at least one candidate server is available, false on DNS or probe failure
 */
bool RobonomicsHTTPAPI::discoverServers() {
	// No point attempting DNS resolution without network connectivity
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
			// Fast path: if we're already registered on the discovery node itself,
			// use it directly — no need to probe further
			if (onServer) {
				selectedHost = resolved.toString();
				return true;
			}

			// If probeServer populated discoveredServers[] from the JSON body,
			// return true so chooseRobonomicsServer() can probe each peer
			if (discoveredCount > 0) return true;

			// The discovery node is reachable but didn't return any peers.
			// Use it as the sole candidate — it's still a valid connectivity server.
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

/**
 * chooseRobonomicsServer — select the best connectivity server for sending data.
 *
 * Implements a three-tier server selection strategy:
 *
 * TIER 1: DNS-based discovery (DISCOVERY_HOST)
 *   - Resolve discovery hostname, probe it, get peer list
 *   - If device is already registered on the discovery node → use it (fastest path)
 *
 * TIER 2: Probe discovered peers
 *   - For each peer from the discovery response, send a GET probe
 *   - If device is already registered on any peer → use it (avoids re-registration)
 *   - Otherwise, pick the peer with fewest sensors (load balancing)
 *
 * TIER 3: Hardcoded fallback (HOST_ROBONOMICS[])
 *   - Only reached if discovery completely fails (DNS down, discovery node unreachable)
 *   - Same logic: prefer registered server, else least-loaded
 *
 * The "prefer registered" logic exists because re-registering a device on a different
 * server wastes bandwidth and may cause temporary data gaps on the map.
 *
 * @return true if selectedHost was set to a valid server, false if everything failed
 */
bool RobonomicsHTTPAPI::chooseRobonomicsServer() {
	// Reset state from previous send cycle — server availability may have changed
	selectedHost = "";
	discoveredCount = 0;

	// TIER 1: Try DNS discovery — if selectedHost was set (device registered on
	// discovery node), we're done
	if (discoverServers() && selectedHost.length() > 0) {
		debug_outln_verbose(F("[Map] Selected server (already registered): "), selectedHost);
		return true;
	}

	// TIER 2: Probe each discovered peer to find the best one
	if (discoveredCount > 0) {
		HTTPClient http;
		http.setTimeout(20 * 1000);
		http.setReuse(false);
		int bestIdx = -1;
		int minSensors = INT_MAX;

		for (int i = 0; i < discoveredCount; i++) {
			// Check WiFi each iteration — probing N servers takes time,
			// and WiFi may drop mid-loop
			if (WiFi.status() != WL_CONNECTED) break;
			int sensors = 0;
			bool onServer = false;
			if (probeServer(http, discoveredServers[i].host, sensors, onServer)) {
				// If already registered here, use it immediately — no need to check the rest
				if (onServer) {
					selectedHost = discoveredServers[i].host;
					debug_outln_verbose(F("[Map] Selected server (registered, discovered): "), selectedHost);
					return true;
				}
				// Track the server with fewest sensors for load-balanced fallback
				if (sensors < minSensors) {
					minSensors = sensors;
					bestIdx = i;
				}
			}
		}
		// No server has us registered — pick the least loaded one
		if (bestIdx >= 0) {
			selectedHost = discoveredServers[bestIdx].host;
			debug_outln_verbose(F("[Map] Selected server (least loaded, discovered): "), selectedHost);
			return true;
		}
	}

	// TIER 3: All discovery failed — fall back to hardcoded server list
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
			// Same priority: prefer a server where we're already registered
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