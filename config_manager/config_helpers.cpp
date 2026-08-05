#include "config_helpers.h"
#include "device_backup.h"
#include "utils.h"
#include "../apis/rws_group.h"
#include "../intl.h"
#include "russia_region_poly.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <strings.h>
#include <cmath>
#include <time.h>
#if defined(ESP32) || defined(ESP8266)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace {

#if defined(ESP32) || defined(ESP8266)
SemaphoreHandle_t g_config_fs_mutex = nullptr;

void ensureConfigFsMutex() {
	if (g_config_fs_mutex == nullptr) {
		g_config_fs_mutex = xSemaphoreCreateRecursiveMutex();
	}
}

class ConfigFsLockGuard {
public:
	explicit ConfigFsLockGuard(uint32_t timeout_ms = portMAX_DELAY) : locked_(false) {
		ensureConfigFsMutex();
		if (g_config_fs_mutex) {
			locked_ = xSemaphoreTakeRecursive(g_config_fs_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
		}
	}
	~ConfigFsLockGuard() {
		if (locked_ && g_config_fs_mutex) {
			xSemaphoreGiveRecursive(g_config_fs_mutex);
		}
	}
	bool ok() const { return locked_; }

private:
	bool locked_;
};
#else
class ConfigFsLockGuard {
public:
	explicit ConfigFsLockGuard(uint32_t = 0) {}
	bool ok() const { return true; }
};
#endif

}  // namespace

#if defined(ALTRUIST_INSIGHT)
#include "../sensors/sensor_names.h"
#include "../display/screens/analytics.h"

void cfgApplyStandaloneModeEnabled() {
	if (cfg::analytics_sleep_add_urban) {
		cfg::analytics_sleep_add_urban = false;
		debug_outln_info(F("Standalone: disabled Urban night analytics"));
	}
	analyticsClearUrbanNightHistory();
}

void cfgOnStandaloneModeDisabled() {
	if (!cfg::analytics_sleep_add_urban) {
		cfg::analytics_sleep_add_urban = true;
		debug_outln_info(F("Paired mode: enabled Urban night analytics"));
	}
}

unsigned cfgMinutesOfDay(unsigned raw, unsigned fallback_minutes) {
	if (raw <= 23u) {
		return raw * 60u;
	}
	if (raw <= 1439u) {
		return raw;
	}
	return fallback_minutes;
}

bool cfgInAnalyticsMorningWindow(const struct tm& timeinfo) {
	if (!cfg::analytics_morning_autoswitch) {
		return false;
	}
	const unsigned now_minutes =
	    static_cast<unsigned>(timeinfo.tm_hour) * 60u + static_cast<unsigned>(timeinfo.tm_min);
	constexpr unsigned kMorningStartMinutes = 6u * 60u;
	const unsigned end_minutes = cfgMinutesOfDay(cfg::analytics_morning_end_hour, 12u * 60u);
	if (end_minutes <= kMorningStartMinutes) {
		return false;
	}
	return now_minutes >= kMorningStartMinutes && now_minutes < end_minutes;
}

void clearUrbanPairingTelemetry(JsonDocument &data) {
	if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
		SPIFFS.remove(F("/urban_ss58.cache"));
	}
	if (!data.isNull() && data.containsKey("service_data")) {
		JsonObject service = data["service_data"].as<JsonObject>();
		if (!service.isNull()) {
			service.remove("urban_robonomics_address");
			service.remove("urban_last_ok_ms");
		}
	}
	data.remove(ATRUIST_URBAN_SENSOR);
	debug_outln_info(F("[Urban] Cleared pairing cache and stale telemetry"));
}
#endif

String getConfigStringValue(const char* key) {
    for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		if (strcmp_P(key, reinterpret_cast<const char*>(c.cfg_key())) == 0) {
            if (c.cfg_type == Config_Type_String) {
                return String(c.cfg_val.as_str);
            } else {
                return "";
            }
        }
	}
    return "";
}

unsigned int getConfigUintValue(const char* key) {
    for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		if (strcmp_P(key, reinterpret_cast<const char*>(c.cfg_key())) == 0) {
            if (c.cfg_type == Config_Type_UInt || c.cfg_type == Config_Type_Time) {
				return static_cast<unsigned int>(*c.cfg_val.as_uint);
            } else {
                return 0;
            }
        }
	}
    return 0;
}

void removeWiFiCredentials() {
	for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());
		if (s_param != "wlanssid" && s_param != "wlanpwd") {
			continue;
		}
		if (s_param == "wlanssid") {
			strncpy(c.cfg_val.as_str, "Not Set", c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
		} else if (s_param == "wlanpwd")
		{
			strncpy(c.cfg_val.as_str, "", c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
		}
		writeConfig();
	}
}

void removeWebUiCredentials() {
	for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());
		if (s_param != "www_username" && s_param != "www_password" && s_param != "www_basicauth_enabled") {
			continue;
		}
		if (s_param == "www_username") {
			strncpy(c.cfg_val.as_str, "admin", c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
		} else if (s_param == "www_password") {
			strncpy(c.cfg_val.as_str, "", c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
		} else if (s_param == "www_basicauth_enabled") {
			*(c.cfg_val.as_bool) = false;
		}
	}
	writeConfig();
}

void saveRobonomicsPrivateKey(const char* private_key) {
	for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());
		if (s_param != "private_key") {
			continue;
		}
		strncpy(c.cfg_val.as_str, private_key, c.cfg_len);
		c.cfg_val.as_str[c.cfg_len] = '\0';
		writeConfig();
	}
}

bool config_set_string_by_key(const char* key, const char* value) {
    for (unsigned i = 0; i < sizeof(configShape) / sizeof(configShape[0]); ++i) {
        ConfigShapeEntry c;
        memcpy_P(&c, &configShape[i], sizeof(ConfigShapeEntry));

        const char* cfg_key_ptr = reinterpret_cast<const char*>(c.cfg_key());
        if (strcmp_P(key, cfg_key_ptr) == 0) {
            if (c.cfg_type == Config_Type_String || c.cfg_type == Config_Type_Password) {
                strncpy(c.cfg_val.as_str, value, c.cfg_len);
                c.cfg_val.as_str[c.cfg_len] = '\0';
                return true;
            } else {
                return false;  // Тип не строковый
            }
        }
    }
    return false;  // Ключ не найден
}

/*****************************************************************
 * write config to spiffs                                        *
 *****************************************************************/

bool writeConfig() {
#if defined(ALTRUIST_INSIGHT)
	if (cfg::standalone) {
		cfg::analytics_sleep_add_urban = false;
	}
#endif
	ConfigFsLockGuard lock(10000);
	if (!lock.ok()) {
		debug_outln_error(F("Config write skipped: filesystem lock busy"));
		return false;
	}

	DynamicJsonDocument json(JSON_BUFFER_SIZE);
	debug_outln_info(F("Saving config..."));
	// Must use to<JsonObject>() — as<JsonObject>() on an empty doc is null and writes nowhere.
	if (!serializeConfigToJson(json.to<JsonObject>())) {
		debug_outln_error(F("Config JSON overflow while saving; increase JSON_BUFFER_SIZE"));
		return false;
	}

	if (json.overflowed()) {
		debug_outln_error(F("Config JSON overflow while saving; increase JSON_BUFFER_SIZE"));
		logSubsystemError(F("config"), F("json_overflow_save"));
		return false;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"

	SPIFFS.remove(F("/config.json.new"));
	File configFile = SPIFFS.open(F("/config.json.new"), "w");
	if (!configFile) {
		debug_outln_error(F("failed to open config file for writing"));
		logSubsystemError(F("config"), F("open_write_failed"), String(F("path=/config.json")));
		return false;
	}
	serializeJson(json, configFile);
	configFile.close();

	SPIFFS.remove(F("/config.json.old"));
	if (SPIFFS.exists(F("/config.json"))) {
		SPIFFS.rename(F("/config.json"), F("/config.json.old"));
	}
	if (!SPIFFS.rename(F("/config.json.new"), F("/config.json"))) {
		debug_outln_error(F("failed to finalize config file write"));
		SPIFFS.remove(F("/config.json.new"));
		return false;
	}
	debug_outln_info(F("Config written successfully."));

#pragma GCC diagnostic pop

	return true;
}

/*****************************************************************
 * read config from spiffs                                       *
 *****************************************************************/

static bool boolFromJSON(const DynamicJsonDocument& json, const __FlashStringHelper* key)
{
	if (json[key].is<char*>()) {
		return !strcmp_P(json[key].as<char*>(), PSTR("true"));
	}
	return json[key].as<bool>();
}

/** OTA-safe: rewrite auto-generated legacy fs_ssid values; keep user-chosen names. */
static bool cfgMigrateLegacyFsSsid() {
	const String chip_id = get_chipid();
	if (chip_id.length() == 0) {
		return false;
	}

	char target[LEN_FS_SSID];
	snprintf(target, sizeof(target), "Altruist-%s-%s", DEVICE_MODEL, chip_id.c_str());
	if (strcmp(cfg::fs_ssid, target) == 0) {
		return false;
	}

	auto is_hex12 = [](const char* s) -> bool {
		if (strlen(s) != 12) {
			return false;
		}
		for (size_t i = 0; s[i] != '\0'; i++) {
			const char c = s[i];
			if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
				return false;
			}
		}
		return true;
	};

	bool migrate = false;
	if (cfg::fs_ssid[0] == '\0') {
		migrate = true;
	} else if (strncasecmp(cfg::fs_ssid, "esp32-", 6) == 0 ||
	           strncasecmp(cfg::fs_ssid, "esp8266-", 8) == 0 ||
	           strncasecmp(cfg::fs_ssid, "robonomics-", 11) == 0) {
		migrate = true;
	} else if (strncmp(cfg::fs_ssid, "Altruist-", 9) == 0) {
		const char* suffix = cfg::fs_ssid + 9;
		// Legacy auto name: Altruist-<12 hex MAC>, not Altruist-insight/urban-...
		if (is_hex12(suffix)) {
			migrate = true;
		}
	}

	if (!migrate) {
		return false;
	}

	debug_outln_info(F("[Config] Migrating fs_ssid from: "), String(cfg::fs_ssid));
	strncpy(cfg::fs_ssid, target, LEN_FS_SSID - 1);
	cfg::fs_ssid[LEN_FS_SSID - 1] = '\0';
	debug_outln_info(F("[Config] Migrated fs_ssid to: "), String(cfg::fs_ssid));
	return true;
}

/** OTA-safe: plain altruist / altruist-insight / altruist-urban → altruist-<model>-<id>; keep custom names. */
static bool cfgMigrateLegacyLocalHostname() {
	const bool legacy =
		cfg::local_hostname[0] == '\0' ||
		strcmp(cfg::local_hostname, "altruist") == 0 ||
		strcmp(cfg::local_hostname, "altruist-insight") == 0 ||
		strcmp(cfg::local_hostname, "altruist-urban") == 0 ||
		strcmp(cfg::local_hostname, LOCAL_HOSTNAME) == 0;
	if (!legacy) {
		return false;
	}
	const String chip_id = get_chipid();
	char target[LEN_LOCAL_HOSTNAME];
	cfg::formatDefaultLocalHostname(target, sizeof(target), chip_id.c_str());
	if (strcmp(cfg::local_hostname, target) == 0) {
		return false;
	}
	debug_outln_info(F("[Config] Migrating local_hostname from: "), String(cfg::local_hostname));
	strncpy(cfg::local_hostname, target, LEN_LOCAL_HOSTNAME - 1);
	cfg::local_hostname[LEN_LOCAL_HOSTNAME - 1] = '\0';
	debug_outln_info(F("[Config] Migrated local_hostname to: "), String(cfg::local_hostname));
	return true;
}

void readConfig(bool oldconfig) {
	ConfigFsLockGuard lock(10000);
	if (!lock.ok()) {
		debug_outln_error(F("Config read skipped: filesystem lock busy"));
		return;
	}

	bool rewriteConfig = false;

	String cfgName(F("/config.json"));
	if (oldconfig) {
		cfgName += F(".old");
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
	File configFile = SPIFFS.open(cfgName, "r");
	if (!configFile) {
		if (!oldconfig) {
			return readConfig(true /* oldconfig */);
		}

		debug_outln_error(F("failed to open config file."));
		logSubsystemError(F("config"), F("open_read_failed"), String(F("path=")) + cfgName);
		return;
	}

	debug_outln_info(F("opened config file..."));
	DynamicJsonDocument json(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json, configFile.readString());
	configFile.close();
#if defined(ALTRUIST_BUILD_DEBUG)
	{
		String saved_private_key;
		if (json.containsKey("private_key")) {
			saved_private_key = json["private_key"].as<const char*>();
			json["private_key"] = "[redacted]";
		}
		serializeJson(json, Serial);
		if (saved_private_key.length() > 0) {
			json["private_key"] = saved_private_key;
		}
	}
#endif
#pragma GCC diagnostic pop

	if (!err) {
		debug_outln_info(F("parsed json..."));
		for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
			ConfigShapeEntry c;
			memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
			if (json[c.cfg_key()].isNull()) {
				continue;
			}
			switch (c.cfg_type) {
			case Config_Type_Bool:
				*(c.cfg_val.as_bool) = boolFromJSON(json, c.cfg_key());
				break;
			case Config_Type_UInt:
			case Config_Type_Time:
				*(c.cfg_val.as_uint) = json[c.cfg_key()].as<unsigned int>();
				break;
			case Config_Type_String:
			case Config_Type_Password:
				strncpy(c.cfg_val.as_str, json[c.cfg_key()].as<char*>(), c.cfg_len);
				c.cfg_val.as_str[c.cfg_len] = '\0';
				break;
			};
		}
		String writtenVersion(json["SOFTWARE_VERSION"].as<char*>());
		if (writtenVersion.length() && writtenVersion[0] == 'N' && String(SOFTWARE_VERSION_STR) != writtenVersion) {
			debug_outln_info(F("Rewriting old config from: "), writtenVersion);
			// would like to do that, but this would wipe firmware.old which the two stage loader
			// might still need
			// SPIFFS.format();
			rewriteConfig = true;
		}
		if (strlen(cfg::measurement_name_influx) == 0) {
			strcpy_P(cfg::measurement_name_influx, MEASUREMENT_NAME_INFLUX);
			rewriteConfig = true;
		}
		if (strcmp_P(cfg::host_influx, PSTR("api.luftdaten.info")) == 0) {
			cfg::host_influx[0] = '\0';
			cfg::send2influx = false;
			rewriteConfig = true;
		}
		if (rwsMigrateLegacyOwnerAtConfigLoad(!json["rws_group_mode"].isNull())) {
			rewriteConfig = true;
		}
		if (cfgMigrateLegacyFsSsid()) {
			rewriteConfig = true;
		}
		if (cfgMigrateLegacyLocalHostname()) {
			rewriteConfig = true;
		}
#if defined(ALTRUIST_INSIGHT)
		if (cfg::standalone && cfg::analytics_sleep_add_urban) {
			cfgApplyStandaloneModeEnabled();
			rewriteConfig = true;
		}
#endif
		// Climate (temp + humidity) is one map chart — keep encrypt flags paired.
		if (cfg::encrypt_temperature != cfg::encrypt_humidity) {
			const bool climate = cfg::encrypt_temperature || cfg::encrypt_humidity;
			cfg::encrypt_temperature = climate;
			cfg::encrypt_humidity = climate;
			rewriteConfig = true;
		}
		if (cfg::leds_brightness > 100) {
			cfg::leds_brightness = 100;
			rewriteConfig = true;
		}
		// Connectivity regions are only RU / Global now; migrate legacy continent ids.
		if (strcmp(cfg::current_reg, REGION_RU) != 0 &&
		    strcmp(cfg::current_reg, REGION_GLOBAL) != 0) {
			strncpy(cfg::current_reg, REGION_GLOBAL, sizeof(cfg::current_reg) - 1);
			cfg::current_reg[sizeof(cfg::current_reg) - 1] = '\0';
			rewriteConfig = true;
		}
		if (cfgApplyAutoRegion()) {
			rewriteConfig = true;
		}
	} else {
		debug_outln_error(F("failed to load json config"));
		logSubsystemError(F("config"), F("json_parse_failed"), String(F("path=")) + cfgName);

		if (!oldconfig) {
			return readConfig(true /* oldconfig */);
		}
	}

	if (rewriteConfig) {
		writeConfig();
	}
}

void init_config() {

	debug_outln_info(F("mounting FS..."));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"

#if defined(ESP32)
	bool spiffs_begin_ok = SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
#else
	bool spiffs_begin_ok = SPIFFS.begin();
#endif

#pragma GCC diagnostic pop

	if (!spiffs_begin_ok) {
		debug_outln_error(F("failed to mount FS"));
		logSubsystemError(F("config"), F("fs_mount_failed"));
		return;
	}
	readConfig();
}

bool cfgHasValidMapCoords(double* lat_out, double* lon_out) {
	double lat = 0.0;
	double lon = 0.0;
	if (strlen(cfg::coords_gps) == 0) {
		return false;
	}
	if (sscanf(cfg::coords_gps, "%lf,%lf", &lat, &lon) != 2) {
		return false;
	}
	if (lat == 0.0 && lon == 0.0) {
		return false;
	}
	if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
		return false;
	}
	if (lat_out) {
		*lat_out = lat;
	}
	if (lon_out) {
		*lon_out = lon;
	}
	return true;
}

static bool cfgPointInRuRing100(int32_t lon100, int32_t lat100, const RuLonLat* pts, uint16_t count) {
	if (!pts || count < 3) {
		return false;
	}
	bool inside = false;
	RuLonLat prev;
	memcpy_P(&prev, &pts[count - 1], sizeof(prev));
	for (uint16_t i = 0; i < count; ++i) {
		RuLonLat cur;
		memcpy_P(&cur, &pts[i], sizeof(cur));
		const int32_t yi = cur.lat100;
		const int32_t yj = prev.lat100;
		if ((yi > lat100) != (yj > lat100)) {
			const int32_t xi = cur.lon100;
			const int32_t xj = prev.lon100;
			const int32_t den = yj - yi;
			if (den != 0) {
				const int32_t xinters = xi + static_cast<int32_t>(
					(static_cast<int64_t>(xj - xi) * (lat100 - yi)) / den);
				if (lon100 < xinters) {
					inside = !inside;
				}
			}
		}
		prev = cur;
	}
	return inside;
}

static bool cfgCoordsInRussia(double lat, double lon) {
	// Cheap reject before point-in-polygon.
	if (lat < 41.0 || lat > 82.5) {
		return false;
	}
	if (!((lon >= 19.0 && lon <= 180.0) || (lon >= -180.0 && lon <= -169.0))) {
		return false;
	}

	const int32_t lon100 = static_cast<int32_t>(lround(lon * 100.0));
	const int32_t lat100 = static_cast<int32_t>(lround(lat * 100.0));
	for (uint8_t r = 0; r < kRuRingCount; ++r) {
		RuRing ring;
		memcpy_P(&ring, &kRuRings[r], sizeof(ring));
		if (cfgPointInRuRing100(lon100, lat100, ring.pts, ring.count)) {
			return true;
		}
	}
	return false;
}

bool cfgApplyAutoRegion() {
	if (cfg::region_manual) {
		return false;
	}

	double lat = 0.0;
	double lon = 0.0;
	if (!cfgHasValidMapCoords(&lat, &lon)) {
		return false;
	}

	const char* want = cfgCoordsInRussia(lat, lon) ? REGION_RU : REGION_GLOBAL;
	if (strcmp(cfg::current_reg, want) == 0) {
		return false;
	}
	strncpy(cfg::current_reg, want, sizeof(cfg::current_reg) - 1);
	cfg::current_reg[sizeof(cfg::current_reg) - 1] = '\0';
	debug_outln_info(F("Region auto-set: "), String(want));
	return true;
}

String buildSensorsSocialMapUrl(const char* sensor_ss58, const char* map_type) {
	if (!map_type || map_type[0] == '\0') {
		map_type = "pm10";
	}
	if (!sensor_ss58 || sensor_ss58[0] == '\0' || strcasecmp(sensor_ss58, "Not Set") == 0) {
		return String(F("https://sensors.social/"));
	}

	char lat[32] = "0.0";
	char lon[32] = "0.0";
	bool coords_ok = false;
	if (strlen(cfg::coords_gps) > 0) {
		if (sscanf(cfg::coords_gps, "%31[^,],%31s", lat, lon) == 2) {
			const double la = atof(lat);
			const double lo = atof(lon);
			coords_ok = !(la == 0.0 && lo == 0.0);
		}
	}
	const int zoom = coords_ok ? 18 : 3;

	char date[11] = "1970-01-01";
	struct tm timeinfo;
	if (getLocalTime(&timeinfo, 0)) {
		strftime(date, sizeof(date), "%Y-%m-%d", &timeinfo);
	}

	const char* owner = sensor_ss58;
	if (cfg::rws_owner[0] != '\0' && strcasecmp(cfg::rws_owner, "Not Set") != 0) {
		owner = cfg::rws_owner;
	}

	char buf[512];
	snprintf(buf, sizeof(buf),
		"https://sensors.social/?type=%s&date=%s&provider=remote&lat=%s&lng=%s&zoom=%d&owner=%s&sensor=%s",
		map_type, date, lat, lon, zoom, owner, sensor_ss58);
	return String(buf);
}
