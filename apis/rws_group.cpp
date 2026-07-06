#include "rws_group.h"
#include "rws_devices_registry.h"
#include "../config_manager/config_helpers.h"
#include "../config_manager/config_defaults.h"
#include "../defines.h"
#include "../utils.h"
#if defined(ESP32)
#include <esp_random.h>
#endif

namespace {

bool ss58Like(const String& address) {
	if (address.length() < 47 || address.length() > 50 || address[0] != '4') {
		return false;
	}
	const char* base58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	for (unsigned i = 0; i < address.length(); ++i) {
		if (!strchr(base58, address[i])) {
			return false;
		}
	}
	return true;
}

bool ownerIsUnset(const char* owner) {
	if (owner == nullptr || owner[0] == '\0') {
		return true;
	}
	String normalized(owner);
	normalized.trim();
	if (normalized.length() == 0) {
		return true;
	}
	normalized.toLowerCase();
	return normalized == "not set";
}

bool ownerLooksLikeAddress(const String& owner) {
	if (ownerIsUnset(owner.c_str())) {
		return false;
	}
	if (ss58Like(owner)) {
		return true;
	}
	return owner.length() >= 40 && owner.length() <= 60;
}

bool groupIdSeedValid(const String& seed) {
	return seed.startsWith(F("GRP-")) &&
	       seed.length() >= 8 &&
	       seed.length() < LEN_RWS_GROUP_ID;
}

void setOwnerInRam(const String& owner) {
	if (ownerLooksLikeAddress(owner)) {
		strncpy(cfg::rws_owner, owner.c_str(), LEN_RWS_OWNER - 1);
	} else {
		strncpy(cfg::rws_owner, "Not Set", LEN_RWS_OWNER - 1);
	}
	cfg::rws_owner[LEN_RWS_OWNER - 1] = '\0';
}

void setKnownDevicesText(const String& text) {
	strncpy(cfg::rws_devices_extra, text.c_str(), LEN_RWS_DEVICES_EXTRA - 1);
	cfg::rws_devices_extra[LEN_RWS_DEVICES_EXTRA - 1] = '\0';
}

}  // namespace

bool rwsSs58Like(const String& address) {
	return ss58Like(address);
}

bool rwsDeviceAddressLike(const String& address) {
	return ownerLooksLikeAddress(address);
}

bool rwsOwnerIsExternal(const String& self_ss58) {
	String owner = String(cfg::rws_owner);
	owner.trim();
	if (!ownerLooksLikeAddress(owner)) {
		return false;
	}
	if (!ss58Like(self_ss58)) {
		return true;
	}
	return owner != self_ss58;
}

bool rwsSyncGroupModeFromOwner(const String& self_ss58) {
	if (cfg::rws_group_mode != RWS_GROUP_STANDALONE || !rwsOwnerIsExternal(self_ss58)) {
		return false;
	}
	cfg::rws_group_mode = RWS_GROUP_MANUAL;
	cfg::rws_auto_register = false;
	if (!writeConfig()) {
		debug_outln_error(F("[RWS] failed to migrate group mode to Manual"));
		return false;
	}
	debug_outln_info(F("[RWS] external owner detected; group mode migrated to Manual"));
	return true;
}

void rwsOnConfigOwnerUpdated(const String& self_ss58) {
	if (cfg::rws_group_mode == RWS_GROUP_FOLLOWER) {
		return;
	}
	if (!rwsOwnerIsExternal(self_ss58)) {
		return;
	}
	cfg::rws_group_mode = RWS_GROUP_MANUAL;
	cfg::rws_auto_register = false;
	cfg::rws_devices_registered_hash[0] = '\0';
}

bool rwsMigrateLegacyOwnerAtConfigLoad(bool group_mode_key_present) {
	if (cfg::rws_group_mode != RWS_GROUP_STANDALONE) {
		return false;
	}

	String owner = String(cfg::rws_owner);
	owner.trim();
	if (ownerIsUnset(owner.c_str())) {
		return false;
	}
	const bool owner_looks_like_address = ownerLooksLikeAddress(owner);
	if (!owner_looks_like_address) {
		return false;
	}

	String hash = String(cfg::rws_devices_registered_hash);
	hash.trim();
	if (hash.length() > 0) {
		String first = hash;
		const int newline = hash.indexOf('\n');
		if (newline >= 0) {
			first = hash.substring(0, newline);
		}
		first.trim();
		if (first == owner) {
			return false;
		}
		cfg::rws_group_mode = RWS_GROUP_MANUAL;
		cfg::rws_auto_register = false;
		debug_outln_info(F("[RWS] legacy owner vs registration hash; mode → Manual"));
		return true;
	}

	if (!group_mode_key_present && owner_looks_like_address) {
		cfg::rws_group_mode = RWS_GROUP_MANUAL;
		cfg::rws_auto_register = false;
		debug_outln_info(F("[RWS] legacy config without rws_group_mode; mode → Manual"));
		return true;
	}

	return false;
}

String rwsGenerateGroupId() {
	char buf[LEN_RWS_GROUP_ID];
#if defined(ESP32)
	snprintf(buf, sizeof(buf), "GRP-%08lx", static_cast<unsigned long>(esp_random()));
#else
	snprintf(buf, sizeof(buf), "GRP-%08lx", static_cast<unsigned long>(millis()));
#endif
	buf[sizeof(buf) - 1] = '\0';
	return String(buf);
}

bool rwsGroupDevicesSynced(const String& self_ss58) {
	return rwsDeviceListMatchesRegistrationHash(self_ss58);
}

bool rwsGroupDevicesSynced(Robonomics* robonomics, const String& self_ss58) {
	if (robonomics != nullptr && rwsDeviceListMatchesRegistrationHash(robonomics)) {
		return true;
	}
	return rwsDeviceListMatchesRegistrationHash(self_ss58);
}

RwsGroupApplyResult rwsApplyGroupSettings(
    unsigned mode,
    const String& self_ss58,
    const String& master_owner,
    const String& known_devices_text,
    const String& manual_owner,
    const String& group_id_seed) {
	if (mode > RWS_GROUP_MANUAL) {
		return RwsGroupApply_InvalidMode;
	}

	switch (mode) {
	case RWS_GROUP_FOLLOWER:
		if (!ownerLooksLikeAddress(master_owner)) {
			debug_outln_error(F("[RWS] follower: invalid master SS58"));
			return RwsGroupApply_InvalidFollowerMaster;
		}
		break;
	case RWS_GROUP_MANUAL:
		if (!ownerLooksLikeAddress(manual_owner)) {
			debug_outln_error(F("[RWS] manual: invalid owner SS58"));
			return RwsGroupApply_InvalidManualOwner;
		}
		break;
	default:
		break;
	}

	const unsigned old_mode = cfg::rws_group_mode;
	const bool mode_changed = old_mode != mode;
	const bool leaving_external_owner =
	    mode_changed && rwsOwnerIsExternal(self_ss58) &&
	    (mode == RWS_GROUP_STANDALONE || mode == RWS_GROUP_MASTER);

	cfg::rws_group_mode = mode;
	cfg::rws_devices_registered_hash[0] = '\0';

	switch (mode) {
	case RWS_GROUP_STANDALONE:
		cfg::rws_group_id[0] = '\0';
		cfg::rws_auto_register = true;
		setKnownDevicesText("");
		if (ownerLooksLikeAddress(self_ss58)) {
			setOwnerInRam(self_ss58);
		}
		break;
	case RWS_GROUP_MASTER:
		if (cfg::rws_group_id[0] == '\0') {
			String gid = group_id_seed;
			if (!groupIdSeedValid(gid)) {
				gid = rwsGenerateGroupId();
			}
			strncpy(cfg::rws_group_id, gid.c_str(), LEN_RWS_GROUP_ID - 1);
			cfg::rws_group_id[LEN_RWS_GROUP_ID - 1] = '\0';
		}
		cfg::rws_auto_register = true;
		setKnownDevicesText(known_devices_text);
		if (ownerLooksLikeAddress(self_ss58)) {
			setOwnerInRam(self_ss58);
		}
		break;
	case RWS_GROUP_FOLLOWER:
		cfg::rws_group_id[0] = '\0';
		cfg::rws_auto_register = false;
		setKnownDevicesText("");
		setOwnerInRam(master_owner);
		break;
	case RWS_GROUP_MANUAL:
		cfg::rws_group_id[0] = '\0';
		cfg::rws_auto_register = false;
		setKnownDevicesText("");
		setOwnerInRam(manual_owner);
		break;
	default:
		return RwsGroupApply_InvalidMode;
	}

	if (!writeConfig()) {
		debug_outln_error(F("[RWS] failed to save group settings"));
		return RwsGroupApply_ConfigWriteFailed;
	}

	if (mode == RWS_GROUP_STANDALONE || mode == RWS_GROUP_MASTER) {
		rwsRequestSetDevicesSubmit();
	}

	if (leaving_external_owner) {
		debug_outln_info(F("[RWS] external owner replaced with device address for group registration"));
	}
	if (mode_changed) {
		debug_outln_info(String(F("[RWS] group mode set to ")) + String(mode));
	}
	return RwsGroupApply_Ok;
}
