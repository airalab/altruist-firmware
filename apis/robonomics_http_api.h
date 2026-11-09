#ifndef __ROBONOMICS_API_H__
#define __ROBONOMICS_API_H__

#include "api.h"
#include "WiFiClient.h"
#include "HTTPClient.h"
#include "../robonomics_servers.h"
#include <Robonomics.h>

static const char URL_ROBONOMICS[] PROGMEM = "/";
#define PORT_ROBONOMICS 65

class RobonomicsHTTPAPI : public API {
public:
  // Constructor with a default timeout value (e.g., 1000 milliseconds)
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
    String connectivity_host_override;
    String connectivity_hosts_pool;
    uint32_t map_send_seq = 0;
    uint32_t map_send_seq_active = 0;
    Robonomics* robonomics;
    void _send(JsonDocument &data) override;
    void POSTRequest(const String& data, const String& host);
    void POSTRequest(const uint8_t *data, size_t len, const String &host);
    int chooseRobonomicsServer(bool onlyGlobal);
    int chooseRobonomicsServerFromPool(const String& pool);
    static int parseHostPool(const String& pool, String* out_hosts, int max_hosts);
    bool formatDataToSend(String &data_to_send, JsonDocument &data);
};

#endif  // __ROBONOMICS_API_H__
