#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <ArduinoJson.h>
#include <WebServer.h>
#include "../wifi_info.h"
#include "../utils.h"

class SensorWebServer {

public:
    SensorWebServer(JsonDocument &_data, device_status_t &_deviceStatus, SemaphoreHandle_t _mutex) : server(80), sensors_data(_data), deviceStatus(_deviceStatus), mutex{_mutex} {}
    void setup();
    /** After STA gets a usable IP again (post-outage); re-open listen socket — some lwIP stacks keep a dead listener. */
    void notifyStaIpRestored();
    void setWifiConfigLoop(bool loop) { wificonfig_loop = loop; };
    /** Single-threaded wrapper around WebServer::handleClient(). */
    void handleClient();
    void setWifiInfo(struct_wifiInfo* info, uint8_t count);
    void setRobonomicsAddress(const String& address) { robonomics_address = address; };

private:
    WebServer server;
    JsonDocument &sensors_data;
    SemaphoreHandle_t mutex;

    device_status_t &deviceStatus;


    String www_password;
    String www_username;
    bool wificonfig_loop = false;
    String robonomics_address;
    String esp_chipid;

    struct_wifiInfo* wifiInfo;
    uint8_t wifiInfoCount;

    bool webserver_request_auth();
    void sendHttpRedirectGuest();
    void sendHttpRedirectConnected(String &address);
    void start_html_page(String& page_content, const String& title);
    void end_html_page(String& page_content);
    void end_html_page_root(String& page_content);
    
    // Web Pages
    void _webserver_guest();
    void _webserver_root();
    void _webserver_config();
    void _webserver_wifi();
    void _webserver_values();
    void _webserver_status();
    void _webserver_debug_level();
    void _webserver_serial();
    void _webserver_removeConfig();
    void _webserver_restart();
    void _webserver_data_json();
    void _webserver_metrics_endpoint();
    void _webserver_favicon();
    void _webserver_static();
    void _webserver_not_found();
    void _webserver_ota();

    void _webserver_group();

#ifdef ALTRUIST_INSIGHT
    void _webserver_screen();
    void _webserver_select_urban();
    void _webserver_scan_urbans();
    /** After user opts to pair: mDNS scan + Urban IP form (no reboot). */
    void _send_urban_pairing_form_html();
#endif
};

#endif // __WEBSERVER_H__