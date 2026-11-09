#ifndef __CUSTOM_API_H__
#define __CUSTOM_API_H__

#include "api.h"
#include "WiFiClient.h"
#include "HTTPClient.h"
#include <Robonomics.h>


class CustomHTTPAPI : public API {
public:
  // Constructor with a default timeout value (e.g., 1000 milliseconds)
  void setup() override;

  void setRobonomcis(Robonomics* robonomics) {
    this->robonomics = robonomics;
  }

private:
    WiFiClient* _client;
    String esp_chipid;
    String rws_owner;
    String host_custom;
    String url_custom;
    uint16_t port_custom;
    Robonomics* robonomics;
    void _send(JsonDocument &data) override;
    bool POSTRequest(const String& data);
    bool formatDataToSend(String &data_to_send, JsonDocument &data);
};

#endif  // __CUSTOM_API_H__
