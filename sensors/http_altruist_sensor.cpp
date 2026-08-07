#ifdef ALTRUIST_INSIGHT

#include "http_altruist_sensor.h"
#include "../utils.h"
#include "../intl.h"
#include "../config_manager/config_helpers.h"
#include "../wifi_manager.h"
#include "sensor_names.h"
#include <WiFi.h>
#include <ESPmDNS.h>

#define HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT 300000UL  // 5 minutes
static const unsigned long HTTP_ALTRUIST_FAST_POLL_MS = 60000UL; // 1 minute until first OK fetch
// Maximum discovery attempts when Urban is not initially present.
// We will try to find Urban up to this many times, spaced 5 minutes apart.
static const uint8_t URBAN_MAX_DISCOVERY_ATTEMPTS = 3;

/** True when config has a manual or previously chosen Urban STA address. */
static bool httpUrbanHasConfiguredAddress() {
    if (cfg::use_custom_urban && strlen(cfg::custom_altruist_urban) > 0) {
        return true;
    }
    return strlen(cfg::chosen_altruist_urban) > 0;
}

/** Bind chosen_address from config (custom IP takes precedence). Returns true if non-empty. */
static String http_urban_last_sta_ip;

static void httpUrbanClearStaleIdentity(JsonDocument &data) {
    if (!data["service_data"].isNull()) {
        JsonObject service = data["service_data"].as<JsonObject>();
        if (!service.isNull()) {
            service.remove("urban_robonomics_address");
        }
    }
    data.remove(ATRUIST_URBAN_SENSOR);
}

static void httpUrbanTrimIp(String &ip) {
    ip.trim();
    // Strip accidental http:// or trailing path from pasted browser URLs.
    if (ip.startsWith(F("http://"))) {
        ip = ip.substring(7);
    } else if (ip.startsWith(F("https://"))) {
        ip = ip.substring(8);
    }
    const int slash = ip.indexOf('/');
    if (slash >= 0) {
        ip = ip.substring(0, slash);
    }
    ip.trim();
}

static bool httpUrbanApplyConfiguredAddress(String &chosen_address) {
    if (cfg::use_custom_urban && strlen(cfg::custom_altruist_urban) > 0) {
        chosen_address = String(cfg::custom_altruist_urban);
        httpUrbanTrimIp(chosen_address);
        debug_outln_verbose(F("HTTPAltruistSensor: using custom_altruist_urban "), chosen_address);
        return chosen_address.length() > 0;
    }
    if (strlen(cfg::chosen_altruist_urban) > 0) {
        chosen_address = String(cfg::chosen_altruist_urban);
        httpUrbanTrimIp(chosen_address);
        debug_outln_verbose(F("HTTPAltruistSensor: using chosen_altruist_urban "), chosen_address);
        return chosen_address.length() > 0;
    }
    chosen_address = "";
    return false;
}

HTTPAltruistSensor::HTTPAltruistSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    if (sending_timeout > HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT) {
        timeout = sending_timeout;
    } else {
        timeout = HTTP_ALTRUIST_FAST_POLL_MS;
    }
    sensor_name = HTTP_ALTRUIST_SENSOR_NAME;
}

bool HTTPAltruistSensor::_discoverSensors() {
    // Bookkeeping for discovery attempts when we don't yet know an Urban IP.
    // We only increment the attempts counter if neither a discovered address
    // nor a configured chosen address is available.
    if (sensor_addresses.empty() && !httpUrbanHasConfiguredAddress()) {
        if (discovery_attempts < URBAN_MAX_DISCOVERY_ATTEMPTS) {
            discovery_attempts++;
        }
    } else {
        // If we already have some address info, reset attempts so future
        // "no Urban" phases can start their own limited sequence.
        discovery_attempts = 0;
    }
    last_discovery_attempt_time = millis();

    sensor_addresses.clear();
    debug_outln_verbose(F("HTTPAltruistSensor: discovering Urban devices via mDNS"));
    int nrOfServices = MDNS.queryService("altruist", "tcp");
   
    if (nrOfServices == 0) {
        debug_outln_info(F("No mDNS Urban services found."));
        if (httpUrbanApplyConfiguredAddress(chosen_address)) {
            last_fetch_time = millis() - timeout;
            return true;
        }
        return false;
    }
    debug_outln_verbose(F("Number of services found: "), String(nrOfServices));

    bool found_chosen = false;

    for (int i = 0; i < nrOfServices; i++) {
        String ip_str = MDNS.address(i).toString();
        String device_type = DEVICE_MODEL_URBAN;
        if (MDNS.hasTxt(i, DEVICE_MODEL_MDNS_PROPERTY)) {
            device_type = MDNS.txt(i, DEVICE_MODEL_MDNS_PROPERTY);
        }

        debug_outln_verbose(F("---------------"));
        debug_outln_verbose(F("Hostname: "), MDNS.hostname(i));
        debug_outln_verbose(F("IP address: "), ip_str);
        debug_outln_verbose(F("Port: "), String(MDNS.port(i)));
        debug_outln_verbose(F("Device type: "), device_type);
        debug_outln_verbose(F("---------------"));

        if (device_type == DEVICE_MODEL_URBAN) {
            sensor_addresses.push_back(ip_str);

            if (ip_str == cfg::chosen_altruist_urban) {
                found_chosen = true;
            }
        }
    }
    if (!cfg::use_custom_urban && sensor_addresses.empty()) {
        debug_outln_info(F("HTTPAltruistSensor: mDNS had no Urban device entries"));
        if (httpUrbanApplyConfiguredAddress(chosen_address)) {
            last_fetch_time = millis() - timeout;
            return true;
        }
        return false;
    }
    if (cfg::use_custom_urban) {
        httpUrbanApplyConfiguredAddress(chosen_address);
    } else {
        if (!found_chosen && !sensor_addresses.empty()) {
            config_set_string_by_key("chosen_altruist_urban", sensor_addresses[0].c_str());
            writeConfig();
            debug_outln_info(F("Chosen altruist sensor not found, using: "), cfg::chosen_altruist_urban);
        }
        chosen_address = String(cfg::chosen_altruist_urban);
    }
    debug_outln_verbose(F("Http Altruis Sensor started with fetch interval (sec): "), String(timeout/1000));
    last_fetch_time = millis() - timeout;
    return true;
}

bool HTTPAltruistSensor::begin() {
    debug_outln_info(F("Begin HTTPAltruistSensor"));
    if (mdns_init() != ESP_OK) {
        if (!httpUrbanHasConfiguredAddress()) {
            debug_outln_info(F("mDNS failed to start"));
            return false;
        }
        debug_outln_info(F("mDNS failed to start; will use configured Urban IP"));
    } else {
        debug_outln_info(F("mDNS init finished"));
    }

    if (!_discoverSensors()) {
        httpUrbanApplyConfiguredAddress(chosen_address);
    }
    // Reset success / failure counters on fresh start
    last_success_time    = 0;
    consecutive_failures = 0;
    return true;
}

void HTTPAltruistSensor::_fetch(JsonDocument &data) {
    // After Insight STA drops and returns, keep using stale chosen_address / DHCP IP blocks Urban until we re-bind.
    static bool http_urban_prev_sta_up = true;
    const bool sta_up = wifiStaLinkReady();
    if (!sta_up) {
        http_urban_prev_sta_up = false;
        return;
    }
    if (!http_urban_prev_sta_up) {
        debug_outln_info(F("HTTPAltruistSensor: WiFi back -> clear Urban bind, retry mDNS/cfg"));
        chosen_address = "";
        consecutive_failures = 0;
    }
    http_urban_prev_sta_up = true;

    debug_outln_verbose(F("fetch HTTP Altruist"));
    // Client before HTTPClient: ~HTTPClient may call _client->stop() even after end()
    // when TCP was already closed (Arduino leaves dangling _client).
    WiFiClient client;
    HTTPClient http;
    JsonObject service = data["service_data"].isNull()
        ? data.createNestedObject("service_data")
        : data["service_data"].as<JsonObject>();
    JsonArray addresses;
    if (service.containsKey("altruist_addresses") && service["altruist_addresses"].is<JsonArray>()) {
        addresses = service["altruist_addresses"].as<JsonArray>();
    } else {
        addresses = service.createNestedArray("altruist_addresses");
    }
    for (const auto& ip_address : sensor_addresses) {
        bool already_exists = false;
        for (JsonVariant v : addresses) {
            if (v.as<String>() == ip_address) {
                already_exists = true;
                break;
            }
        }
        if (!already_exists) {
            addresses.add(ip_address);
        }
    }

    // If we don't yet know which Urban IP to use, try to (re)discover it
    // or fall back to the last configured IP before attempting an HTTP
    // request. This avoids calling HTTP with an empty host and makes sure we give Urban multiple chances to appear.
    if (chosen_address.length() == 0) {
        // 1) Configured custom or chosen IP (works without mDNS, e.g. ESP32-C3 Urban).
        if (!httpUrbanApplyConfiguredAddress(chosen_address)) {
            // 2) No configured IP: drive mDNS rediscovery up to a limited number of attempts, spaced in time.
            if (discovery_attempts < URBAN_MAX_DISCOVERY_ATTEMPTS) {
                bool first_attempt     = (discovery_attempts == 0);
                bool interval_elapsed = (last_discovery_attempt_time == 0) ||
                                        (msSince(last_discovery_attempt_time) >= URBAN_REDISCOVER_INTERVAL_MS);

                if (first_attempt || interval_elapsed) {
                    debug_outln_verbose(F("HTTPAltruistSensor: proactive rediscovery from _fetch, attempt "),
                                     String(discovery_attempts + 1));
                    if (_discoverSensors()) {
                        consecutive_failures = 0;
                    } else {
                        httpUrbanApplyConfiguredAddress(chosen_address);
                    }
                }
            }
        }
    }

    // If we still don't have a target Urban address, skip this cycle.
    if (chosen_address.length() == 0) {
        return;
    }

    if (chosen_address != http_urban_last_sta_ip) {
        debug_outln_info(F("HTTPAltruistSensor: Urban target IP changed to "), chosen_address);
        httpUrbanClearStaleIdentity(data);
        http_urban_last_sta_ip = chosen_address;
    }

    // WiFiClient `client` (declared above) must outlive `http`.
    _fetch_one_sensor(data, http, client, chosen_address);
    http.end();
    // sensor_name = HTTP_ALTRUIST_SENSOR_NAME;
}

void HTTPAltruistSensor::_fetch_one_sensor(JsonDocument &data, HTTPClient& http, WiFiClient& client,
					   const String &ip_address) {
    String target_ip = ip_address;
    httpUrbanTrimIp(target_ip);
    debug_outln_verbose(F("fetch HTTP Altruist "), target_ip);

    IPAddress urban_ip;
    const bool have_ip = urban_ip.fromString(target_ip);

    // One TCP probe: separates "LAN block" from HTTPClient quirks. Phone OK + TCP fail => router path.
    {
        WiFiClient probe;
        probe.setTimeout(2000);
        bool tcp_ok = false;
        if (have_ip) {
            tcp_ok = probe.connect(urban_ip, 80);
        } else {
            tcp_ok = probe.connect(target_ip.c_str(), 80);
        }
        if (tcp_ok) {
            probe.stop();
            debug_outln_verbose(F("HTTPAltruistSensor: TCP :80 OK -> "), target_ip);
        } else {
            debug_outln_info(F("HTTPAltruistSensor: TCP :80 FAIL -> "), target_ip);
            debug_outln_info(F("  Insight IP "), WiFi.localIP().toString());
            debug_outln_info(F("  gateway "), WiFi.gatewayIP().toString());
        }
    }

    // A few quick GETs help when the LAN path to Urban is flaky (mesh / ARP / brief isolation).
    int httpCode = -1;
    const int kMaxAttempts = 3;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        if (attempt > 0) {
            http.end();
            client.stop();
            delay(400);
            debug_outln_info(F("HTTPAltruistSensor: retry Urban GET "), String(attempt + 1));
        }
        http.setReuse(false);
        http.setTimeout(12000);
        // Prefer host/port/uri + shared client (more reliable than URL string on ESP32).
        bool began = false;
        if (have_ip) {
            began = http.begin(client, target_ip, 80, JSON_DATA_PATH, false);
        } else {
            began = http.begin(client, SENSOR_URL_PREFIX + target_ip + JSON_DATA_PATH);
        }
        if (!began) {
            httpCode = HTTPC_ERROR_CONNECTION_REFUSED;
            continue;
        }
        httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            break;
        }
    }

    if (httpCode == HTTP_CODE_OK) {
        debug_outln_verbose(F("Success request to Altruis Urban"));

        {
            JsonObject service = data["service_data"].isNull()
                ? data.createNestedObject("service_data")
                : data["service_data"].as<JsonObject>();
            service["urban_last_ok_ms"] = (uint32_t)millis();
        }
        debug_outln_info(F("[Urban][TTL] HTTP OK -> ttl updated"));

        // Ensure we have a dedicated Urban block in the global sensors_data:
        // "altruist_urban": { IP_address, SDS_P1, SDS_P2, ... }
        // It is pre-created at boot in setup(), but if for some reason it's missing, create it here without clearing the rest of sensors_data.
        JsonObject urbanRoot = data[ATRUIST_URBAN_SENSOR];
        if (urbanRoot.isNull()) {
            debug_outln_info(F("HTTPAltruistSensor: altruist_urban missing in sensors_data, creating on the fly"));
            urbanRoot = data.createNestedObject(ATRUIST_URBAN_SENSOR);
            if (urbanRoot.isNull()) {
                debug_outln_info(F("HTTPAltruistSensor: FAILED to create altruist_urban (JSON memory issue)"));
#if defined(ALTRUIST_BUILD_DEBUG)
                serializeJson(data, Serial);
#endif
                http.end();
                return;
            }
        }

        // Store IP address inside the Urban block in the same "value/int_name/units" shape
        {
            JsonObject ipObj = urbanRoot["IP_address"];
            if (ipObj.isNull()) {
                ipObj = urbanRoot.createNestedObject("IP_address");
                ipObj[F("intl_name")] = INTL_IP_ADDRESS;
                ipObj[F("units")]     = "";
            }
            if (!ipObj[F("value")].isNull()) {
                const char *prev = ipObj[F("value")].as<const char*>();
                if (prev && strcmp(prev, target_ip.c_str()) == 0) {
                    // unchanged
                } else {
                    ipObj[F("value")] = target_ip;
                }
            } else {
                ipObj[F("value")] = target_ip;
            }
        }

        // Parse Urban's /data.json payload and map sensordatavalues into altruist_urban
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            debug_outln_info(F("[Urban][TTL] JSON parse error (ttl kept fresh): "), err.c_str());
            http.end();
            return;
        }

        JsonArray values = doc["sensordatavalues"];
        debug_outln_verbose(F("HTTPAltruistSensor: sensordatavalues count "),
                         String(values.size()));

        bool seen_sds_p1 = false;
        bool seen_sds_p2 = false;
        for (JsonObject v : values) {
            String type  = v["value_type"];
            float  value = v["value"].as<float>();

            // Пример соответствия intl_name и units
            String units;
            String intl_name;
            if (type == "SDS_P1") {
                seen_sds_p1 = true;
                intl_name = "PM10";
                units = F("µg/m³");
            } else if (type == "SDS_P2") {
                seen_sds_p2 = true;
                intl_name = "PM2.5";
                units = F("µg/m³");
            } else if (type == "BME280_temperature") {
                intl_name = INTL_TEMPERATURE;
                units = F("°C");
            } else if (type == "BME280_humidity") {
                intl_name = INTL_HUMIDITY;
                units = F("%");
            } else if (type == "BME280_pressure") {
                intl_name = INTL_PRESSURE;
                units = F("Pa");
            } else if (type == "PCBA_noiseMax") {
                intl_name = INTL_NOISE_MAX;
                units = F("db");
            } else if (type == "PCBA_noiseAvg") {
                intl_name = INTL_NOISE_MEAN;
                units = F("db");
            } else {
                // Unknown/extra types are still stored so they are visible in JSON
                intl_name = type;
                units = "";
            }

            JsonObject measObj = urbanRoot[type];
            if (measObj.isNull()) {
                measObj = urbanRoot.createNestedObject(type);
                measObj[F("intl_name")] = intl_name;
                measObj[F("units")]     = units;
            }
            measObj[F("value")] = value;
        }
        if (!seen_sds_p1) {
            urbanRoot.remove("SDS_P1");
        }
        if (!seen_sds_p2) {
            urbanRoot.remove("SDS_P2");
        }
        // Mark JSON as updated so SD card logger (and graph data) see Urban data
        _jsonUpdated = true;

        // Capture Urban device's Robonomics address from data.json, or fallback to HTML extraction
        bool has_urban_addr = false;
        if (doc.containsKey("service_data") && doc["service_data"].containsKey("robonomics_address")) {
            String urban_addr = doc["service_data"]["robonomics_address"].as<String>();
            if (urban_addr.length() > 0) {
                JsonObject service = data["service_data"].isNull() ? data.createNestedObject("service_data") : data["service_data"].as<JsonObject>();
                const char *prev = service["urban_robonomics_address"].as<const char*>();
                if (!prev || strcmp(prev, urban_addr.c_str()) != 0) {
                    service["urban_robonomics_address"] = urban_addr;
                }
                has_urban_addr = true;
            }
        }
        if (!has_urban_addr) {
            HTTPClient http2;
            String root_url = SENSOR_URL_PREFIX + target_ip + String("/");
            http2.begin(root_url);
            http2.setTimeout(12000);
            int httpCode2 = http2.GET();
            if (httpCode2 == HTTP_CODE_OK) {
                String html = http2.getString();
                // Fallback heuristic: first base58-like sequence starting with '4'
                const char *base58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
                for (size_t i = 0; i < html.length(); ++i) {
                    if (html[i] == '4') {
                        size_t j = i;
                        while (j < html.length() && strchr(base58, html[j])) {
                            ++j;
                        }
                        size_t len = j - i;
                        if (len >= 47 && len <= 50) { // accept typical SS58 length range
                            String urban_addr = html.substring(i, j);
                            JsonObject service = data["service_data"].isNull() ? data.createNestedObject("service_data") : data["service_data"].as<JsonObject>();
                            const char *prev = service["urban_robonomics_address"].as<const char*>();
                            if (!prev || strcmp(prev, urban_addr.c_str()) != 0) {
                                service["urban_robonomics_address"] = urban_addr;
                            }
                            break;
                        }
                    }
                }
            }
            http2.end();
        }
        #if defined(ALTRUIST_BUILD_DEBUG)
        serializeJson(data, Serial);
        #endif

        // Mark successful communication with Urban
        last_success_time    = millis();
        consecutive_failures = 0;
        if (timeout < HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT) {
            timeout = HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT;
        }
        
        // IMPORTANT: Close the HTTP connection to free resources
        http.end();
    } else {
        if (httpCode < 0) {
            debug_outln_info(F("Request to Altruist Urban failed: "), HTTPClient::errorToString(httpCode));
            debug_outln_info(F("  Insight cannot open LAN path to Urban (phone may still work)"));
        } else {
            debug_outln_info(F("Request to Altruist Urban failed, HTTP "), httpCode);
        }
        consecutive_failures++;
        // While failing, poll every ~1 min instead of waiting 5 min between tries.
        timeout = HTTP_ALTRUIST_FAST_POLL_MS;

        const bool have_configured_ip = httpUrbanHasConfiguredAddress();
        bool never_succeeded = (last_success_time == 0);
        bool have_any_address = !sensor_addresses.empty() || have_configured_ip;

        if (never_succeeded && !have_any_address) {
            // Limited discovery sequence while Urban is "possibly not present".
            if (discovery_attempts < URBAN_MAX_DISCOVERY_ATTEMPTS) {
                bool first_attempt = (discovery_attempts == 0);
                bool interval_elapsed = msSince(last_discovery_attempt_time) >= URBAN_REDISCOVER_INTERVAL_MS;

                if (first_attempt || interval_elapsed) {
                    debug_outln_info(F("HTTPAltruistSensor: scheduled rediscovery attempt "), String(discovery_attempts + 1));
                    if (_discoverSensors()) {
                        consecutive_failures = 0;
                    }
                }
            } else {
                debug_outln_info(F("HTTPAltruistSensor: reached max discovery attempts, Urban assumed absent"));
            }
        } else if (!have_configured_ip) {
            // No saved IP: mDNS rediscovery may find Urban after it joins Wi‑Fi.
            bool long_since_success = (last_success_time != 0 && msSince(last_success_time) > URBAN_REDISCOVER_INTERVAL_MS);
            if (long_since_success) {
                debug_outln_info(F("HTTPAltruistSensor: attempting rediscovery after prolonged failures"));
                if (_discoverSensors()) {
                    consecutive_failures = 0;
                }
            }
            const unsigned long FAST_REDISCOVER_THROTTLE_MS = 30UL * 1000UL;
            bool fast_interval_elapsed = (last_discovery_attempt_time == 0) ||
                                         (msSince(last_discovery_attempt_time) >= FAST_REDISCOVER_THROTTLE_MS);
            if (fast_interval_elapsed) {
                debug_outln_info(F("HTTPAltruistSensor: fast rediscovery after HTTP failure"));
                _discoverSensors();
            }
        } else if (!cfg::use_custom_urban) {
            bool long_since_success = (last_success_time != 0 && msSince(last_success_time) > URBAN_REDISCOVER_INTERVAL_MS);
            bool interval_elapsed = (last_discovery_attempt_time == 0) ||
                                    (msSince(last_discovery_attempt_time) >= URBAN_REDISCOVER_INTERVAL_MS);
            if (long_since_success && interval_elapsed) {
                debug_outln_info(F("HTTPAltruistSensor: occasional mDNS check (saved IP still preferred)"));
                _discoverSensors();
            }
        }
        
        // Close the HTTP connection even on failure
        http.end();
    }
}

#endif
