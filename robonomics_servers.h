#ifndef __ROBONOMICS_SERVERS_H__
#define __ROBONOMICS_SERVERS_H__

#include "./intl.h"

// Global Servers - REGION_GLOBAL
// Europe - REGION_EU
// Asia - REGION_AS
// Africa - REGION_AF
// Australia - REGION_AU
// North America - REGION_NA
// South America - REGION_SA


static const char* const DISCOVERY_HOST PROGMEM = "discovery.connectivity.robonomics.network";

static const char* const HOST_ROBONOMICS[][2] PROGMEM = {
                                                    {"connectivity.robonomics.network", REGION_GLOBAL},
                                                    {"1.connectivity.robonomics.network", REGION_GLOBAL}, 
                                                    {"2.connectivity.robonomics.network", REGION_GLOBAL},
                                                    };



#endif // __ROBONOMICS_SERVERS_H__  