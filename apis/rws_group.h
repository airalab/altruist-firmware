#ifndef RWS_GROUP_H
#define RWS_GROUP_H

#include <Arduino.h>

class Robonomics;

enum RwsGroupApplyResult : uint8_t {
	RwsGroupApply_None = 0,
	RwsGroupApply_Ok,
	RwsGroupApply_InvalidMode,
	RwsGroupApply_InvalidFollowerMaster,
	RwsGroupApply_InvalidManualOwner,
	RwsGroupApply_ConfigWriteFailed,
};

String rwsGenerateGroupId();
bool rwsSs58Like(const String& address);

/** SS58 or similar Robonomics address */
bool rwsDeviceAddressLike(const String& address);

/** True when cfg::rws_owner is a valid SS58 different from this device. */
bool rwsOwnerIsExternal(const String& self_ss58);

/**
 * external owner + default Standalone mode → Manual.
 * Persists when a change is made. Safe to call before registry / UI.
 */
bool rwsSyncGroupModeFromOwner(const String& self_ss58);

/** After /config saves rws_owner: switch to Manual when owner is external. */
void rwsOnConfigOwnerUpdated(const String& self_ss58);

/**
 * migrate Standalone → Manual before any owner sync.
 * Returns true when cfg was changed in RAM (caller should writeConfig).
 */
bool rwsMigrateLegacyOwnerAtConfigLoad(bool group_mode_key_present);

bool rwsGroupDevicesSynced(const String& self_ss58);

/** Prefer robonomics when available (same device list as set_devices). */
bool rwsGroupDevicesSynced(Robonomics* robonomics, const String& self_ss58);

/**
 * Apply group mode from /group form.
 * Returns result code (Ok when config was written successfully).
 */
RwsGroupApplyResult rwsApplyGroupSettings(
    unsigned mode,
    const String& self_ss58,
    const String& master_owner,
    const String& known_devices_text,
    const String& manual_owner,
    const String& group_id_seed = String());

#endif
