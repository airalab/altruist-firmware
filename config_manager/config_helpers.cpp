#include "config_helpers.h"
#include "utils.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>


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
	DynamicJsonDocument json(JSON_BUFFER_SIZE);
	debug_outln_info(F("Saving config..."));
	json["SOFTWARE_VERSION"] = SOFTWARE_VERSION_STR;

	for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		switch (c.cfg_type) {
		case Config_Type_Bool:
			json[c.cfg_key()].set(*c.cfg_val.as_bool);
			break;
		case Config_Type_UInt:
		case Config_Type_Time:
			json[c.cfg_key()].set(*c.cfg_val.as_uint);
			break;
		case Config_Type_Password:
		case Config_Type_String:
			json[c.cfg_key()].set(c.cfg_val.as_str);
			break;
		};
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"

	SPIFFS.remove(F("/config.json.old"));
	SPIFFS.rename(F("/config.json"), F("/config.json.old"));

	File configFile = SPIFFS.open(F("/config.json"), "w");
	if (configFile) {
		serializeJson(json, configFile);
		configFile.close();
		debug_outln_info(F("Config written successfully."));
	} else {
		debug_outln_error(F("failed to open config file for writing"));
		return false;
	}
	configFile.close();

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

void readConfig(bool oldconfig) {
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
		return;
	}

	debug_outln_info(F("opened config file..."));
	DynamicJsonDocument json(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json, configFile.readString());
	configFile.close();
	serializeJson(json, Serial);
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
	} else {
		debug_outln_error(F("failed to load json config"));

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
		return;
	}
	readConfig();
}