#ifndef __ROBONOMICS_API_H__
#define __ROBONOMICS_API_H__

#include "api.h"
#include "WiFiClient.h"
#include "HTTPClient.h"
#include "../robonomics_servers.h"
#include <Robonomics.h>

static const char URL_ROBONOMICS[] PROGMEM = "/";
#define PORT_ROBONOMICS 65
#define MAX_DISCOVERED_SERVERS 10

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
    String selectedHost;
    DiscoveredServer discoveredServers[MAX_DISCOVERED_SERVERS];
    int discoveredCount = 0;
    void _send(JsonDocument &data) override;
    void POSTRequest(const String& data, const char* host);
    void POSTRequest(const String& data, const String& host);
    bool chooseRobonomicsServer();
    bool discoverServers();
    bool probeServer(HTTPClient& http, const String& host, int& sensors, bool& onServer);
    void formatDataToSend(String &data_to_send, JsonDocument &data);
};

#endif  // __ROBONOMICS_API_H__