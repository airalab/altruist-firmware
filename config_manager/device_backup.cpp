#include "device_backup.h"
#include "config_helpers.h"
#include "airrohr-cfg.h"
#include "../defines.h"
#include <strings.h>

static bool ownerKeyAvailable(const String& robonomics_address) {
	const char* sk = cfg::private_key;
	return sk && strcasecmp(sk, "Not Set") != 0 && strlen(sk) >= 64 &&
	       robonomics_address.length() > 0 && strcasecmp(robonomics_address.c_str(), "Not Set") != 0;
}

bool serializeConfigToJson(JsonObject json) {
	if (json.isNull()) {
		return false;
	}
	json["SOFTWARE_VERSION"] = SOFTWARE_VERSION_STR;

	for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e) {
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
		}
	}
	return true;
}

bool applyConfigFromJson(JsonObjectConst json) {
	for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		if (json[c.cfg_key()].isNull()) {
			continue;
		}
		switch (c.cfg_type) {
		case Config_Type_Bool:
			if (json[c.cfg_key()].is<bool>()) {
				*(c.cfg_val.as_bool) = json[c.cfg_key()].as<bool>();
			} else if (json[c.cfg_key()].is<const char*>()) {
				*(c.cfg_val.as_bool) = strcmp(json[c.cfg_key()].as<const char*>(), "true") == 0;
			}
			break;
		case Config_Type_UInt:
		case Config_Type_Time:
			*(c.cfg_val.as_uint) = json[c.cfg_key()].as<unsigned int>();
			break;
		case Config_Type_String:
		case Config_Type_Password:
			strncpy(c.cfg_val.as_str, json[c.cfg_key()].as<const char*>(), c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
			break;
		}
	}
	return true;
}

static void appendOwnerSection(JsonObject owner, const String& robonomics_address) {
	owner["format"] = "altruist-owner1";
	owner["type"] = "ed25519";
	owner["address"] = robonomics_address;
	owner["seed"] = cfg::private_key;
	owner["sensor"] = robonomics_address;
	owner["hint"] = "Import this backup on sensors.map Login to decrypt self-owner encrypted metrics";
}

bool buildDeviceBackupJson(String& out, const String& robonomics_address, const String& hostname, bool include_owner_key) {
	DynamicJsonDocument doc(JSON_BUFFER_SIZE);
	doc["format"] = "altruist-backup1";

	JsonObject device = doc.createNestedObject("device");
	device["address"] = robonomics_address;
	device["hostname"] = hostname;
	device["model"] = DEVICE_MODEL;

	if (include_owner_key && ownerKeyAvailable(robonomics_address)) {
		JsonObject owner = doc.createNestedObject("owner");
		appendOwnerSection(owner, robonomics_address);
	}

	JsonObject config = doc.createNestedObject("config");
	if (!serializeConfigToJson(config)) {
		return false;
	}

	if (doc.overflowed()) {
		return false;
	}

	serializeJson(doc, out);
	return true;
}

DeviceBackupRestoreResult restoreDeviceBackupFromJson(const JsonDocument& doc) {
	if (!doc["format"].is<const char*>() || strcmp(doc["format"], "altruist-backup1") != 0) {
		return DeviceBackupRestoreResult::UnsupportedFormat;
	}
	JsonObjectConst config = doc["config"];
	if (config.isNull()) {
		return DeviceBackupRestoreResult::MissingConfig;
	}
	if (!applyConfigFromJson(config)) {
		return DeviceBackupRestoreResult::ConfigOverflow;
	}

#if defined(ALTRUIST_INSIGHT)
	if (cfg::standalone) {
		cfg::analytics_sleep_add_urban = false;
	}
#endif

	if (!writeConfig()) {
		return DeviceBackupRestoreResult::WriteFailed;
	}
	return DeviceBackupRestoreResult::Ok;
}
