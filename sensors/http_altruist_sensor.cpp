#ifdef ALTRUIST_INSIGHT

#include "http_altruist_sensor.h"
#include "../utils.h"
#include "../intl.h"
#include "../config_manager/config_helpers.h"
#include "../wifi_manager.h"
#include "sensor_names.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <vector>
#include <cstring>

#define HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT 300000UL  // 5 minutes
static const unsigned long HTTP_ALTRUIST_FAST_POLL_MS = 60000UL; // 1 minute until first OK fetch
static const uint8_t URBAN_MAX_DISCOVERY_ATTEMPTS = 3;
static ExtraUrbanItem g_extras[MAX_EXTRA_URBANS];
static uint8_t g_extras_n = 0;

static void httpUrbanMainIp(char *out, size_t out_len) {
    extraUrbansMainIp(out, out_len);
}

static void refreshExtras() {
    g_extras_n = extraUrbansParse(g_extras, MAX_EXTRA_URBANS);
}

static bool extraUrbanSlotUsable(uint8_t slot, const char *main_ip) {
    if (slot >= g_extras_n || g_extras[slot].ip[0] == '\0') {
        return false;
    }
    if (main_ip && main_ip[0] != '\0' && strcmp(g_extras[slot].ip, main_ip) == 0) {
        return false;
    }
    return true;
}

static uint8_t extraUrbanCount() {
    refreshExtras();
    char main_ip[40];
    httpUrbanMainIp(main_ip, sizeof(main_ip));
    uint8_t n = 0;
    for (uint8_t i = 0; i < g_extras_n; i++) {
        if (extraUrbanSlotUsable(i, main_ip)) {
            n++;
        }
    }
    return n;
}

static uint8_t httpUrbanPageCount() {
    if (cfg::standalone) {
        return 1;
    }
    return (uint8_t)(1 + extraUrbanCount());
}

static bool extraUrbanNth(uint8_t want, uint8_t *slot_out, const char **ip_out, const char **name_out) {
    refreshExtras();
    char main_ip[40];
    httpUrbanMainIp(main_ip, sizeof(main_ip));
    uint8_t seen = 0;
    for (uint8_t i = 0; i < g_extras_n; i++) {
        if (!extraUrbanSlotUsable(i, main_ip)) {
            continue;
        }
        if (seen == want) {
            if (slot_out) *slot_out = i;
            if (ip_out) *ip_out = g_extras[i].ip;
            if (name_out) *name_out = g_extras[i].name;
            return true;
        }
        seen++;
    }
    return false;
}

/** True when config has a manual or previously chosen Urban STA address. */
static bool httpUrbanHasConfiguredAddress() {
    if (cfg::use_custom_urban && strlen(cfg::custom_altruist_urban) > 0) {
        return true;
    }
    if (strlen(cfg::chosen_altruist_urban) > 0) {
        return true;
    }
    return extraUrbanCount() > 0;
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

static HTTPAltruistSensor *g_http_urban = nullptr;
static uint8_t g_urban_page = 0;

static void httpUrbanSetMeas(JsonObject urbanRoot, const char *type, const char *intl,
                             const __FlashStringHelper *units, float value, bool present) {
    if (urbanRoot.isNull()) {
        return;
    }
    if (!present) {
        urbanRoot.remove(type);
        return;
    }
    JsonObject measObj = urbanRoot[type];
    if (measObj.isNull()) {
        measObj = urbanRoot.createNestedObject(type);
        measObj[F("intl_name")] = intl;
        measObj[F("units")] = units;
    }
    measObj[F("value")] = value;
}

HTTPAltruistSensor::HTTPAltruistSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    g_http_urban = this;
    if (sending_timeout > HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT) {
        timeout = sending_timeout;
    } else {
        timeout = HTTP_ALTRUIST_FAST_POLL_MS;
    }
    sensor_name = HTTP_ALTRUIST_SENSOR_NAME;
}

void HTTPAltruistSensor::requestImmediateFetch() {
    last_fetch_time = millis() - timeout;
}

void HTTPAltruistSensor::_capAddressList() {
    if (sensor_addresses.size() > kMaxUrbans) {
        sensor_addresses.resize(kMaxUrbans);
    }
}

void HTTPAltruistSensor::_ensureExtraUrbans() {
    refreshExtras();
    char main_ip[40];
    httpUrbanMainIp(main_ip, sizeof(main_ip));
    for (uint8_t slot = 0; slot < g_extras_n; slot++) {
        if (!extraUrbanSlotUsable(slot, main_ip)) {
            continue;
        }
        const String ip = g_extras[slot].ip;
        bool have = false;
        for (const auto &a : sensor_addresses) {
            if (a == ip) {
                have = true;
                break;
            }
        }
        if (!have && sensor_addresses.size() < kMaxUrbans) {
            sensor_addresses.push_back(ip);
        }
        _ensureCacheSlot(ip);
    }
    }

int HTTPAltruistSensor::_cacheIndexForIp(const String &ip) const {
    for (uint8_t i = 0; i < kMaxUrbans; ++i) {
        if (urban_cache[i].ip == ip && urban_cache[i].ip.length() > 0) {
            return i;
        }
    }
    return -1;
}

int HTTPAltruistSensor::_ensureCacheSlot(const String &ip) {
    int existing = _cacheIndexForIp(ip);
    if (existing >= 0) {
        return existing;
    }
    for (uint8_t i = 0; i < kMaxUrbans; ++i) {
        if (urban_cache[i].ip.length() == 0) {
            urban_cache[i] = UrbanSnap{};
            urban_cache[i].ip = ip;
            return i;
        }
    }
    urban_cache[0] = UrbanSnap{};
    urban_cache[0].ip = ip;
    return 0;
}

void HTTPAltruistSensor::_writePager(JsonDocument &data) {
    JsonObject service = data["service_data"].isNull()
        ? data.createNestedObject("service_data")
        : data["service_data"].as<JsonObject>();
    if (service.isNull()) {
        return;
    }
    const uint8_t total = sensor_addresses.empty() ? (uint8_t)0 : (uint8_t)sensor_addresses.size();
    service["urban_view_total"] = total;
    service["urban_view_index"] = (total == 0) ? (uint8_t)0 : (uint8_t)(view_index + 1);
}

void HTTPAltruistSensor::_applySnap(JsonDocument &data, uint8_t cache_index) {
    if (cache_index >= kMaxUrbans || !urban_cache[cache_index].valid) {
        return;
    }
    const UrbanSnap &snap = urban_cache[cache_index];
    JsonObject urbanRoot = data[ATRUIST_URBAN_SENSOR];
    if (urbanRoot.isNull()) {
        urbanRoot = data.createNestedObject(ATRUIST_URBAN_SENSOR);
        if (urbanRoot.isNull()) {
            return;
        }
    }
    {
        JsonObject ipObj = urbanRoot["IP_address"];
        if (ipObj.isNull()) {
            ipObj = urbanRoot.createNestedObject("IP_address");
            ipObj[F("intl_name")] = INTL_IP_ADDRESS;
            ipObj[F("units")] = "";
        }
        ipObj[F("value")] = snap.ip;
    }
    httpUrbanSetMeas(urbanRoot, "SDS_P1", "PM10", F("µg/m³"), snap.pm10, snap.pm10 >= 0);
    httpUrbanSetMeas(urbanRoot, "SDS_P2", "PM2.5", F("µg/m³"), snap.pm25, snap.pm25 >= 0);
    httpUrbanSetMeas(urbanRoot, "BME280_temperature", INTL_TEMPERATURE, F("°C"), snap.temp, snap.temp > -999);
    httpUrbanSetMeas(urbanRoot, "BME280_humidity", INTL_HUMIDITY, F("%"), snap.hum, snap.hum >= 0);
    httpUrbanSetMeas(urbanRoot, "BME280_pressure", INTL_PRESSURE, F("Pa"), snap.press, snap.press >= 0);
    httpUrbanSetMeas(urbanRoot, "PCBA_noiseMax", INTL_NOISE_MAX, F("db"), snap.noise_max, snap.noise_max >= 0);
    httpUrbanSetMeas(urbanRoot, "PCBA_noiseAvg", INTL_NOISE_MEAN, F("db"), snap.noise_avg, snap.noise_avg >= 0);

    JsonObject service = data["service_data"].isNull()
        ? data.createNestedObject("service_data")
        : data["service_data"].as<JsonObject>();
    if (!service.isNull()) {
        service["urban_last_ok_ms"] = snap.last_ok_ms;
        if (snap.ss58.length() > 0) {
            service["urban_robonomics_address"] = snap.ss58;
        } else {
            service.remove("urban_robonomics_address");
        }
    }
}

bool HTTPAltruistSensor::cycleView() {
    _ensureExtraUrbans();
    if (sensor_addresses.size() < 2) {
        return false;
    }
    view_index = (uint8_t)((view_index + 1) % sensor_addresses.size());
    chosen_address = sensor_addresses[view_index];
    http_urban_last_sta_ip = chosen_address;
    requestImmediateFetch();
    debug_outln_info(F("HTTPAltruistSensor: cycle Urban -> "), chosen_address);
    return true;
}

bool HTTPAltruistSensor::cycleViewNoWrap() {
    _ensureExtraUrbans();
    if (sensor_addresses.size() < 2 || (view_index + 1) >= sensor_addresses.size()) {
        return false;
    }
    view_index = (uint8_t)(view_index + 1);
    chosen_address = sensor_addresses[view_index];
    http_urban_last_sta_ip = chosen_address;
    requestImmediateFetch();
    debug_outln_info(F("HTTPAltruistSensor: next Urban -> "), chosen_address);
    return true;
}

bool HTTPAltruistSensor::cyclePrevNoWrap() {
    _ensureExtraUrbans();
    if (sensor_addresses.size() < 2 || view_index == 0) {
        return false;
    }
    view_index = (uint8_t)(view_index - 1);
    chosen_address = sensor_addresses[view_index];
    http_urban_last_sta_ip = chosen_address;
    requestImmediateFetch();
    debug_outln_info(F("HTTPAltruistSensor: prev Urban -> "), chosen_address);
    return true;
}

void HTTPAltruistSensor::syncToJson(JsonDocument &data) {
    // Graphs / SD / LEDs / web values always see the configured main Urban.
    // Extra Urbans stay in cache and are only overlaid on the MAIN e-paper pages.
    char main_ip[40];
    httpUrbanMainIp(main_ip, sizeof(main_ip));
    int slot = -1;
    if (main_ip[0] != '\0') {
        slot = _cacheIndexForIp(String(main_ip));
    }
    if (slot < 0) {
        slot = _cacheIndexForIp(chosen_address);
    }
    if (slot >= 0 && urban_cache[slot].valid) {
        _applySnap(data, (uint8_t)slot);
    }
    _writePager(data);
}

uint8_t HTTPAltruistSensor::pagerIndex() const {
    const uint8_t total = pagerTotal();
    if (total == 0) {
        return 0;
    }
    return (uint8_t)(view_index + 1);
}

uint8_t HTTPAltruistSensor::pagerTotal() const {
    return sensor_addresses.empty() ? (uint8_t)0 : (uint8_t)sensor_addresses.size();
}

bool HTTPAltruistSensor::copyCurrentSnap(float *pm10, float *pm25, float *noise_avg, float *noise_max,
                                         float *temp, float *hum, float *press_pa, char *ip, size_t ip_len) const {
    if (chosen_address.length() == 0) {
        return false;
    }
    const int slot = _cacheIndexForIp(chosen_address);
    if (slot < 0 || !urban_cache[slot].valid) {
        return false;
    }
    const UrbanSnap &snap = urban_cache[slot];
    if (pm10) *pm10 = snap.pm10;
    if (pm25) *pm25 = snap.pm25;
    if (noise_avg) *noise_avg = snap.noise_avg;
    if (noise_max) *noise_max = snap.noise_max;
    if (temp) *temp = snap.temp;
    if (hum) *hum = snap.hum;
    if (press_pa) *press_pa = snap.press;
    if (ip && ip_len > 0) {
        strncpy(ip, snap.ip.c_str(), ip_len - 1);
        ip[ip_len - 1] = '\0';
    }
    return true;
}

bool HTTPAltruistSensor::copySnapForIp(const String &ip, float *pm10, float *pm25, float *noise_avg, float *noise_max,
                                       float *temp, float *hum, float *press_pa) const {
    if (ip.length() == 0) {
        return false;
    }
    const int slot = _cacheIndexForIp(ip);
    if (slot < 0 || !urban_cache[slot].valid) {
        return false;
    }
    const UrbanSnap &snap = urban_cache[slot];
    if (pm10) *pm10 = snap.pm10;
    if (pm25) *pm25 = snap.pm25;
    if (noise_avg) *noise_avg = snap.noise_avg;
    if (noise_max) *noise_max = snap.noise_max;
    if (temp) *temp = snap.temp;
    if (hum) *hum = snap.hum;
    if (press_pa) *press_pa = snap.press;
    return true;
}

bool HTTPAltruistSensor::cycleNext(JsonDocument &data) {
    if (!cycleView()) {
        return false;
    }
    syncToJson(data);
    return true;
}

bool httpUrbanCycleNext(JsonDocument &data) {
    if (!g_http_urban) {
        return false;
    }
    return g_http_urban->cycleNext(data);
}

bool httpUrbanCycleNext() {
    if (!g_http_urban) {
        return false;
    }
    return g_http_urban->cycleView();
}

void httpUrbanSyncToJson(JsonDocument &data) {
    if (!g_http_urban) {
        return;
    }
    g_http_urban->syncToJson(data);
}

bool httpUrbanCanCycleOnMain() {
    return httpUrbanPageCount() > 1;
}

void httpUrbanResetToMainPage() {
    if (g_urban_page == 0) {
        return;
    }
    g_urban_page = 0;
    debug_outln_info(F("HTTPAltruistSensor: MAIN urban page reset to 1"));
}

bool httpUrbanCycleNextView() {
    const uint8_t n = httpUrbanPageCount();
    if (n <= 1) {
        return true;
    }
    if (g_urban_page + 1 >= n) {
        return true;
    }
    g_urban_page++;
    debug_outln_info(F("HTTPAltruistSensor: MAIN urban page "), String(g_urban_page + 1));
    return false;
}

bool httpUrbanCyclePrevView() {
    const uint8_t n = httpUrbanPageCount();
    if (n <= 1 || g_urban_page == 0) {
        return true;
    }
    g_urban_page--;
    debug_outln_info(F("HTTPAltruistSensor: MAIN urban page "), String(g_urban_page + 1));
    return false;
}

bool httpUrbanGetOutdoorView(uint8_t *index1, uint8_t *total, bool *snap_valid,
                             float *pm10, float *pm25, float *noise_avg, float *noise_max,
                             float *temp, float *hum, float *press_pa, char *ip, size_t ip_len,
                             char *name, size_t name_len, bool *solo) {
    const uint8_t pages = httpUrbanPageCount();
    if (g_urban_page >= pages) {
        g_urban_page = 0;
    }
    if (index1) *index1 = (uint8_t)(g_urban_page + 1);
    if (total) *total = pages;
    if (name && name_len > 0) {
        name[0] = '\0';
    }
    if (solo) *solo = (g_urban_page > 0);

    if (cfg::standalone) {
        if (snap_valid) *snap_valid = false;
        return false;
    }

    if (g_urban_page == 0) {
        if (solo) *solo = false;
        char main_ip[40];
        httpUrbanMainIp(main_ip, sizeof(main_ip));
        if (ip && ip_len > 0 && main_ip[0] != '\0') {
            strncpy(ip, main_ip, ip_len - 1);
            ip[ip_len - 1] = '\0';
        }
        bool ok = false;
        if (g_http_urban) {
            if (main_ip[0] != '\0') {
                ok = g_http_urban->copySnapForIp(String(main_ip), pm10, pm25, noise_avg, noise_max, temp, hum, press_pa);
            }
            if (!ok) {
                ok = g_http_urban->copyCurrentSnap(pm10, pm25, noise_avg, noise_max, temp, hum, press_pa, ip, ip_len);
            }
        }
        if (snap_valid) *snap_valid = ok;
        return true;
    }

    const char *extra_ip = nullptr;
    const char *extra_name = nullptr;
    if (!extraUrbanNth((uint8_t)(g_urban_page - 1), nullptr, &extra_ip, &extra_name)) {
        if (snap_valid) *snap_valid = false;
        return true;
    }
    if (ip && ip_len > 0 && extra_ip) {
        strncpy(ip, extra_ip, ip_len - 1);
        ip[ip_len - 1] = '\0';
    }
    if (name && name_len > 0 && extra_name && extra_name[0] != '\0') {
        strncpy(name, extra_name, name_len - 1);
        name[name_len - 1] = '\0';
    }
    bool ok = false;
    if (g_http_urban && extra_ip) {
        ok = g_http_urban->copySnapForIp(String(extra_ip), pm10, pm25, noise_avg, noise_max, temp, hum, press_pa);
    }
    if (snap_valid) *snap_valid = ok;
    return true;
}

bool HTTPAltruistSensor::_discoverSensors() {
    struct EnsureExtrasOnExit {
        HTTPAltruistSensor *self;
        ~EnsureExtrasOnExit() {
            if (self) {
                self->_ensureExtraUrbans();
            }
        }
    } ensure_extras{this};

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

    std::vector<String> previous_addresses = sensor_addresses;
    sensor_addresses.clear();
    debug_outln_verbose(F("HTTPAltruistSensor: discovering Urban devices via mDNS"));
    int nrOfServices = MDNS.queryService("altruist", "tcp");
   
    if (nrOfServices == 0) {
        debug_outln_info(F("No mDNS Urban services found."));
        if (!cfg::use_custom_urban && !previous_addresses.empty()) {
            sensor_addresses = previous_addresses;
            last_fetch_time = millis() - timeout;
            return true;
        }
        if (httpUrbanApplyConfiguredAddress(chosen_address)) {
            sensor_addresses.push_back(chosen_address);
            view_index = 0;
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
            if (sensor_addresses.size() < kMaxUrbans) {
                bool already = false;
                for (const auto &known : sensor_addresses) {
                    if (known == ip_str) {
                        already = true;
                        break;
                    }
                }
                if (!already) {
                    sensor_addresses.push_back(ip_str);
                }
            }

            if (ip_str == cfg::chosen_altruist_urban) {
                found_chosen = true;
            }
        }
    }
    if (!cfg::use_custom_urban) {
        for (const auto &ip : previous_addresses) {
            if (sensor_addresses.size() >= kMaxUrbans) {
                break;
            }
            bool have = false;
            for (const auto &known : sensor_addresses) {
                if (known == ip) {
                    have = true;
                    break;
                }
            }
            if (!have) {
                sensor_addresses.push_back(ip);
            }
        }
    }
    const String keep_view = chosen_address;
    if (!cfg::use_custom_urban && sensor_addresses.empty()) {
        debug_outln_info(F("HTTPAltruistSensor: mDNS had no Urban device entries"));
        if (!previous_addresses.empty()) {
            sensor_addresses = previous_addresses;
            last_fetch_time = millis() - timeout;
            return true;
        }
        if (httpUrbanApplyConfiguredAddress(chosen_address)) {
            sensor_addresses.push_back(chosen_address);
            view_index = 0;
            last_fetch_time = millis() - timeout;
            return true;
        }
        return false;
    }
    if (cfg::use_custom_urban) {
        httpUrbanApplyConfiguredAddress(chosen_address);
        sensor_addresses.clear();
        if (chosen_address.length() > 0) {
            sensor_addresses.push_back(chosen_address);
        }
        view_index = 0;
    } else {
        String cfg_ip;
        if (httpUrbanApplyConfiguredAddress(cfg_ip) && cfg_ip.length() > 0) {
            bool have_cfg = false;
            for (const auto &ip : sensor_addresses) {
                if (ip == cfg_ip) {
                    have_cfg = true;
                    break;
                }
            }
            if (!have_cfg && sensor_addresses.size() < kMaxUrbans) {
                sensor_addresses.insert(sensor_addresses.begin(), cfg_ip);
            }
        }
        _capAddressList();
        view_index = 0;
        bool restored_view = false;
        if (keep_view.length() > 0) {
            for (size_t i = 0; i < sensor_addresses.size(); ++i) {
                if (sensor_addresses[i] == keep_view) {
                    view_index = (uint8_t)i;
                    restored_view = true;
                    break;
                }
            }
        }
        if (!restored_view && found_chosen) {
            for (size_t i = 0; i < sensor_addresses.size(); ++i) {
                if (sensor_addresses[i] == String(cfg::chosen_altruist_urban)) {
                    view_index = (uint8_t)i;
                    break;
                }
            }
        }
        if (!found_chosen && !sensor_addresses.empty() && strlen(cfg::chosen_altruist_urban) == 0) {
            config_set_string_by_key("chosen_altruist_urban", sensor_addresses[0].c_str());
            writeConfig();
            debug_outln_info(F("Chosen altruist sensor not found, using: "), cfg::chosen_altruist_urban);
        } else if (!found_chosen && !sensor_addresses.empty() && keep_view.length() == 0) {
            config_set_string_by_key("chosen_altruist_urban", sensor_addresses[0].c_str());
            writeConfig();
            debug_outln_info(F("Chosen altruist sensor not found, using: "), cfg::chosen_altruist_urban);
        }
        if (!sensor_addresses.empty()) {
            chosen_address = sensor_addresses[view_index];
        }
    }
    _ensureExtraUrbans();
    if (!sensor_addresses.empty() && view_index < sensor_addresses.size()) {
        chosen_address = sensor_addresses[view_index];
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
    _ensureExtraUrbans();
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
    _ensureExtraUrbans();

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
    if (chosen_address.length() == 0 && !sensor_addresses.empty()) {
        if (view_index >= sensor_addresses.size()) {
            view_index = 0;
        }
        chosen_address = sensor_addresses[view_index];
    }

    // SD / LEDs / web always log the configured main Urban. Extra Urbans are
    // cache-only so paging them on the display cannot pollute graph CSV.
    char main_ip[40];
    httpUrbanMainIp(main_ip, sizeof(main_ip));
    String json_ip;
    if (main_ip[0] != '\0') {
        json_ip = String(main_ip);
    } else if (chosen_address.length() > 0) {
        json_ip = chosen_address;
    } else if (!sensor_addresses.empty()) {
        json_ip = sensor_addresses[0];
    }
    if (json_ip.length() == 0) {
        return;
    }
    httpUrbanTrimIp(json_ip);

    if (json_ip != http_urban_last_sta_ip) {
        debug_outln_info(F("HTTPAltruistSensor: Urban target IP changed to "), json_ip);
        httpUrbanClearStaleIdentity(data);
        http_urban_last_sta_ip = json_ip;
    }

    _writePager(data);

    // WiFiClient `client` (declared above) must outlive `http`.
    _fetch_one_sensor(data, http, client, json_ip, true, false);
    http.end();
    client.stop();

    const int main_slot = _cacheIndexForIp(json_ip);
    if (main_slot >= 0 && urban_cache[main_slot].valid) {
        _applySnap(data, (uint8_t)main_slot);
    }

    if (sensor_addresses.size() > 1) {
        const size_t n = sensor_addresses.size();
        uint8_t bg = (uint8_t)((view_index + 1 + bg_rotate) % n);
        if (bg == view_index) {
            bg = (uint8_t)((bg + 1) % n);
        }
        // Never apply an extra Urban into sensors_data (that is what SD logs).
        if (sensor_addresses[bg] != json_ip) {
            bg_rotate++;
            HTTPClient http_bg;
            _fetch_one_sensor(data, http_bg, client, sensor_addresses[bg], false, true);
            http_bg.end();
        }
    }
}

void HTTPAltruistSensor::_fetch_one_sensor(JsonDocument &data, HTTPClient& http, WiFiClient& client,
					   const String &ip_address, bool apply_to_json, bool quick) {
    String target_ip = ip_address;
    httpUrbanTrimIp(target_ip);
    debug_outln_verbose(F("fetch HTTP Altruist "), target_ip);

    IPAddress urban_ip;
    const bool have_ip = urban_ip.fromString(target_ip);

    if (!quick) {
        // One TCP probe: separates "LAN block" from HTTPClient quirks. Phone OK + TCP fail => router path.
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
    const int kMaxAttempts = quick ? 1 : 3;
    const int kHttpTimeout = quick ? 4000 : 12000;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        if (attempt > 0) {
            http.end();
            client.stop();
            delay(400);
            debug_outln_info(F("HTTPAltruistSensor: retry Urban GET "), String(attempt + 1));
        }
        http.setReuse(false);
        http.setTimeout(kHttpTimeout);
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

        UrbanSnap parsed;
        parsed.ip = target_ip;
        parsed.valid = true;
        parsed.last_ok_ms = (uint32_t)millis();
        const int prev_slot = _cacheIndexForIp(target_ip);
        if (prev_slot >= 0) {
            parsed.ss58 = urban_cache[prev_slot].ss58;
        }

        bool seen_sds_p1 = false;
        bool seen_sds_p2 = false;
        for (JsonObject v : values) {
            String type  = v["value_type"];
            float  value = v["value"].as<float>();

            if (type == "SDS_P1") {
                seen_sds_p1 = true;
                parsed.pm10 = value;
            } else if (type == "SDS_P2") {
                seen_sds_p2 = true;
                parsed.pm25 = value;
            } else if (type == "BME280_temperature") {
                parsed.temp = value;
            } else if (type == "BME280_humidity") {
                parsed.hum = value;
            } else if (type == "BME280_pressure") {
                parsed.press = value;
            } else if (type == "PCBA_noiseMax") {
                parsed.noise_max = value;
            } else if (type == "PCBA_noiseAvg") {
                parsed.noise_avg = value;
            }

            if (!apply_to_json) {
                continue;
            }

            String units;
            String intl_name;
            if (type == "SDS_P1") {
                intl_name = "PM10";
                units = F("µg/m³");
            } else if (type == "SDS_P2") {
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
                intl_name = type;
                units = "";
            }

            JsonObject urbanRoot = data[ATRUIST_URBAN_SENSOR];
            if (urbanRoot.isNull()) {
                urbanRoot = data.createNestedObject(ATRUIST_URBAN_SENSOR);
                if (urbanRoot.isNull()) {
                    continue;
                }
            }
            JsonObject measObj = urbanRoot[type];
            if (measObj.isNull()) {
                measObj = urbanRoot.createNestedObject(type);
                measObj[F("intl_name")] = intl_name;
                measObj[F("units")]     = units;
            }
            measObj[F("value")] = value;
        }

        if (doc.containsKey("service_data") && doc["service_data"].containsKey("robonomics_address")) {
            String urban_addr = doc["service_data"]["robonomics_address"].as<String>();
            if (urban_addr.length() > 0) {
                parsed.ss58 = urban_addr;
            }
        }

        if (apply_to_json) {
            JsonObject service = data["service_data"].isNull()
                ? data.createNestedObject("service_data")
                : data["service_data"].as<JsonObject>();
            service["urban_last_ok_ms"] = parsed.last_ok_ms;
            debug_outln_info(F("[Urban][TTL] HTTP OK -> ttl updated"));

            JsonObject urbanRoot = data[ATRUIST_URBAN_SENSOR];
            if (urbanRoot.isNull()) {
                debug_outln_info(F("HTTPAltruistSensor: altruist_urban missing in sensors_data, creating on the fly"));
                urbanRoot = data.createNestedObject(ATRUIST_URBAN_SENSOR);
            }
            if (urbanRoot.isNull()) {
                debug_outln_info(F("HTTPAltruistSensor: FAILED to create altruist_urban (JSON memory issue)"));
#if defined(ALTRUIST_BUILD_DEBUG)
                serializeJson(data, Serial);
#endif
                http.end();
                return;
            }
            {
                JsonObject ipObj = urbanRoot["IP_address"];
                if (ipObj.isNull()) {
                    ipObj = urbanRoot.createNestedObject("IP_address");
                    ipObj[F("intl_name")] = INTL_IP_ADDRESS;
                    ipObj[F("units")]     = "";
                }
                ipObj[F("value")] = target_ip;
            }
            if (!seen_sds_p1) {
                urbanRoot.remove("SDS_P1");
            }
            if (!seen_sds_p2) {
                urbanRoot.remove("SDS_P2");
            }
            _jsonUpdated = true;

            if (parsed.ss58.length() > 0) {
                const char *prev = service["urban_robonomics_address"].as<const char*>();
                if (!prev || strcmp(prev, parsed.ss58.c_str()) != 0) {
                    service["urban_robonomics_address"] = parsed.ss58;
                }
            }
        }

        // HTML fallback only for the Urban currently shown — skip on background cache fills.
        if (apply_to_json && parsed.ss58.length() == 0) {
            HTTPClient http2;
            String root_url = SENSOR_URL_PREFIX + target_ip + String("/");
            http2.begin(root_url);
            http2.setTimeout(12000);
            int httpCode2 = http2.GET();
            if (httpCode2 == HTTP_CODE_OK) {
                String html = http2.getString();
                const char *base58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
                for (size_t i = 0; i < html.length(); ++i) {
                    if (html[i] == '4') {
                        size_t j = i;
                        while (j < html.length() && strchr(base58, html[j])) {
                            ++j;
                        }
                        size_t len = j - i;
                        if (len >= 47 && len <= 50) {
                            parsed.ss58 = html.substring(i, j);
                            JsonObject service = data["service_data"].isNull() ? data.createNestedObject("service_data") : data["service_data"].as<JsonObject>();
                            const char *prev = service["urban_robonomics_address"].as<const char*>();
                            if (!prev || strcmp(prev, parsed.ss58.c_str()) != 0) {
                                service["urban_robonomics_address"] = parsed.ss58;
                            }
                            break;
                        }
                    }
                }
            }
            http2.end();
        }

        const int slot = _ensureCacheSlot(target_ip);
        urban_cache[slot] = parsed;
        urban_cache[slot].ip = target_ip;

        #if defined(ALTRUIST_BUILD_DEBUG)
        if (apply_to_json) {
            serializeJson(data, Serial);
        }
        #endif

        if (apply_to_json) {
            last_success_time    = millis();
            consecutive_failures = 0;
            if (timeout < HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT) {
                timeout = HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT;
            }
        }

        http.end();
    } else {
        if (httpCode < 0) {
            debug_outln_info(F("Request to Altruist Urban failed: "), HTTPClient::errorToString(httpCode));
            debug_outln_info(F("  Insight cannot open LAN path to Urban (phone may still work)"));
        } else {
            debug_outln_info(F("Request to Altruist Urban failed, HTTP "), httpCode);
        }
        if (!apply_to_json) {
            http.end();
            return;
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
