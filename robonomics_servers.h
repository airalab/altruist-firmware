#ifndef __ROBONOMICS_SERVERS_H__
#define __ROBONOMICS_SERVERS_H__

#include "./intl.h"

// Built-in sensors.social connectivity pool (region-tagged).
// Custom host / host pool in settings overrides this list.
// OTA updates these compiled defaults; a filled config field is kept.

#define ROBONOMICS_PROTO_CONNECTIVITY_HOST_STR "http://128.140.46.75/v1/telemetry"

static const char* const HOST_ROBONOMICS[][2] PROGMEM = {
	{"connectivity.robonomics.network", REGION_RU},
	{"1.connectivity.robonomics.network", REGION_GLOBAL},
	{"2.connectivity.robonomics.network", REGION_GLOBAL},
};

// Protobuf ingest, same region tags as the JSON pool. Not posted to :65/.
static const char* const HOST_ROBONOMICS_PROTO[][2] PROGMEM = {
	{ROBONOMICS_PROTO_CONNECTIVITY_HOST_STR, REGION_RU},
	{ROBONOMICS_PROTO_CONNECTIVITY_HOST_STR, REGION_GLOBAL},
};

static const char ROBONOMICS_PROTO_CONNECTIVITY_HOST[] PROGMEM =
	ROBONOMICS_PROTO_CONNECTIVITY_HOST_STR;

#endif // __ROBONOMICS_SERVERS_H__
