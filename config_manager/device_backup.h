#ifndef __DEVICE_BACKUP_H__
#define __DEVICE_BACKUP_H__

#include <Arduino.h>
#include <ArduinoJson.h>

enum class DeviceBackupRestoreResult : uint8_t {
	Ok = 0,
	InvalidJson,
	UnsupportedFormat,
	MissingConfig,
	ConfigOverflow,
	WriteFailed,
};

bool serializeConfigToJson(JsonObject json);
bool applyConfigFromJson(JsonObjectConst json);
bool buildDeviceBackupJson(String& out, const String& robonomics_address, const String& hostname, bool include_owner_key);
DeviceBackupRestoreResult restoreDeviceBackupFromJson(const JsonDocument& doc);

#endif // __DEVICE_BACKUP_H__
