#pragma once

#include <Arduino.h>
#include <functional>

void improv_serial_setup();
void improv_serial_loop();

using ImprovWifiCallback = std::function<bool(const String& ssid, const String& password)>;
using ImprovRwsOwnerCallback = std::function<void(const String& rws_owner)>;

void improv_set_wifi_callback(ImprovWifiCallback cb);
void improv_set_rws_owner_callback(ImprovRwsOwnerCallback cb);

void improv_set_state(uint8_t state);
void improv_set_error(uint8_t error);
void improv_send_response(const std::vector<String>& datum);
void improv_start_announce(uint8_t count);