#ifndef __ROBONOMICS_SERVERS_H__
#define __ROBONOMICS_SERVERS_H__

#include "./intl.h"

// Built-in sensors.social connectivity pool (region-tagged).
// Custom host / host pool in settings overrides this list.

static const char* const HOST_ROBONOMICS[][2] PROGMEM = {
	{"connectivity.robonomics.network", REGION_RU},
	{"1.connectivity.robonomics.network", REGION_GLOBAL},
	{"2.connectivity.robonomics.network", REGION_GLOBAL},
};

#endif // __ROBONOMICS_SERVERS_H__