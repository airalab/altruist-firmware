#ifndef __ROBONOMICS_SERVERS_H__
#define __ROBONOMICS_SERVERS_H__

#include "./intl.h"

// Region codes used to tag each server with its geographic location.
// Global Servers - REGION_GLOBAL
// Europe - REGION_EU
// Asia - REGION_AS
// Africa - REGION_AF
// Australia - REGION_AU
// North America - REGION_NA
// South America - REGION_SA

/**
 * DISCOVERY_HOST — DNS hostname of the Robonomics discovery service.
 *
 * This is the entry point for dynamic server discovery. The firmware resolves
 * this hostname via DNS to find the IP of a discovery node. That node responds
 * with a list of available connectivity servers (peers) in its HTTP response body.
 *
 * Using a dedicated discovery hostname (instead of hardcoded IPs) allows the
 * infrastructure team to add/remove/rebalance servers without firmware updates.
 * DNS round-robin or geo-DNS can also direct devices to the nearest discovery node.
 *
 * Stored in PROGMEM to keep it in flash and save RAM on ESP32.
 */
static const char* const DISCOVERY_HOST PROGMEM = "discovery.connectivity.robonomics.network";

/**
 * HOST_ROBONOMICS — hardcoded fallback server list.
 *
 * Used as a last resort when DNS-based discovery fails (e.g., DNS timeout,
 * discovery service is down, or no peers returned). Each entry is a pair of
 * {hostname, region}. The region tag is currently REGION_GLOBAL for all entries
 * but allows future region-aware routing.
 *
 * The order matters: connectivity.robonomics.network is tried first as the
 * primary/load-balanced endpoint, then numbered instances (1., 2.) as direct
 * fallbacks to specific servers.
 */
static const char* const HOST_ROBONOMICS[][2] PROGMEM = {
                                                    {"connectivity.robonomics.network", REGION_GLOBAL},
                                                    {"1.connectivity.robonomics.network", REGION_GLOBAL},
                                                    {"2.connectivity.robonomics.network", REGION_GLOBAL},
                                                    };



#endif // __ROBONOMICS_SERVERS_H__