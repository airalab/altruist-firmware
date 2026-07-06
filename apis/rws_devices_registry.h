#ifndef __RWS_DEVICES_REGISTRY_H__
#define __RWS_DEVICES_REGISTRY_H__

#include <Arduino.h>

class Robonomics;

/** Clear stale registration hash when owner was never saved (no config.json edit needed). */
void repairInconsistentRwsRegistrationState();

/** Register RWS device list on-chain via rws.set_devices (also syncs owner when needed). */
void ensureRwsDevicesRegistered(Robonomics* robonomics);

/** Clear registration hash so the next worker pass submits set_devices. */
void rwsClearRegistrationHash();

/** Queue set_devices on the next registry tick (also clears registration hash). */
void rwsRequestSetDevicesSubmit();

/** Expected on-chain device list fingerprint from current config. */
String rwsBuildExpectedDeviceFingerprint(const String& self_address);
String rwsBuildExpectedDeviceFingerprint(Robonomics* robonomics);

/** True when cfg::rws_devices_registered_hash matches the expected device list. */
bool rwsDeviceListMatchesRegistrationHash(Robonomics* robonomics);
bool rwsDeviceListMatchesRegistrationHash(const String& self_address);

#endif
