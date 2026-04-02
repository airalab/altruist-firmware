#ifndef __ROBONOMICS_API_H__
#define __ROBONOMICS_API_H__

#include "api.h"
#include "WiFiClient.h"
#include "HTTPClient.h"
#include "../robonomics_servers.h"
#include <Robonomics.h>

static const char URL_ROBONOMICS[] PROGMEM = "/";
#define PORT_ROBONOMICS 65

/**
 * MAX_DISCOVERED_SERVERS — upper limit on how many peer servers we store from discovery.
 *
 * Set to 10 because: the discovery service typically returns 2-5 peers, so 10 gives
 * comfortable headroom. Keeping it small avoids excessive RAM use on ESP32 (each
 * DiscoveredServer holds a String + int + bool, roughly ~20-30 bytes on heap).
 */
#define MAX_DISCOVERED_SERVERS 10

/**
 * DiscoveredServer — represents a single connectivity server found via discovery.
 *
 * @field host      IP address or hostname of the server (e.g., "185.200.100.50")
 * @field sensors   Number of sensors currently connected to this server; used to
 *                  pick the least-loaded server for load balancing
 * @field onServer  True if THIS device is already registered on that server;
 *                  if true, we prefer this server to avoid re-registration overhead
 */
struct DiscoveredServer {
    String host;
    int sensors;
    bool onServer;
};

class RobonomicsHTTPAPI : public API {
public:
  void setup() override;

  void setRobonomcis(Robonomics* robonomics) {
    this->robonomics = robonomics;
  }

private:
    WiFiClient* _client;
    String esp_chipid;
    String current_reg;
    String donated_by;
    String rws_owner;
    Robonomics* robonomics;
    String selectedHost;                                      // The host chosen by chooseRobonomicsServer() for sending data
    DiscoveredServer discoveredServers[MAX_DISCOVERED_SERVERS]; // Buffer for servers returned by discovery
    int discoveredCount = 0;                                   // Number of valid entries in discoveredServers[]

    void _send(JsonDocument &data) override;
    void POSTRequest(const String& data, const char* host);
    void POSTRequest(const String& data, const String& host);

    /**
     * chooseRobonomicsServer — main server selection logic.
     *
     * Tries three strategies in order:
     *   1. DNS discovery (DISCOVERY_HOST) — if device is already registered there, use it immediately
     *   2. Probe discovered peers — prefer one where device is registered, else pick least-loaded
     *   3. Hardcoded fallback (HOST_ROBONOMICS[]) — same logic: prefer registered, else least-loaded
     *
     * @return true if a server was selected (stored in selectedHost), false if all failed
     */
    bool chooseRobonomicsServer();

    /**
     * discoverServers — resolve DISCOVERY_HOST via DNS and probe it for peer list.
     *
     * On success, populates discoveredServers[] with peers from the JSON response,
     * or sets selectedHost directly if the device is already registered on the discovery node.
     *
     * @return true if discovery yielded at least one usable server, false on failure
     */
    bool discoverServers();

    /**
     * probeServer — send a GET request to a single server to check availability and metadata.
     *
     * Reads custom HTTP headers "sensors-count" and "on-server" from the response,
     * and parses the JSON body for a "servers" array of peer nodes.
     *
     * @param http      Reusable HTTPClient instance (caller manages lifecycle)
     * @param host      IP or hostname to probe
     * @param sensors   [out] Number of sensors reported by this server
     * @param onServer  [out] Whether this device is already registered on that server
     * @return true if the server responded with 2xx, false otherwise
     */
    bool probeServer(HTTPClient& http, const String& host, int& sensors, bool& onServer);

    void formatDataToSend(String &data_to_send, JsonDocument &data);
};

#endif  // __ROBONOMICS_API_H__