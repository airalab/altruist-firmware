#ifndef API_H
#define API_H

#include "Arduino.h"
#include <ArduinoJson.h>
#include "../utils.h"

class API {
protected:
    unsigned long timeout;  // Private variable for API timeout
    unsigned long last_send_time = 0;
    time_t last_send_time_t = 0;
    unsigned long count_sends = 0;
    unsigned long count_sends_success = 0;
    bool is_ok = true;
    api_status_t new_api_status;

    void updateSendTime() {
        last_send_time = millis();
        count_sends++;
        if (is_ok) {
            last_send_time_t = time(nullptr);
            count_sends_success++;
        }
    }
    virtual void _send(JsonDocument &data) = 0;


public:

    virtual ~API() {}
    virtual void setup() = 0;

    const char* api_name;

    void updateDeviceStatus(device_status_t &deviceStatus) {
        debug_outln_info(F("Updating device status for "), api_name);
        if (deviceStatus.apis_status.find(api_name) == deviceStatus.apis_status.end()) {
            debug_outln_info(F("Creating new API status"));
            deviceStatus.apis_status[api_name] = new_api_status;
            debug_outln_info(F("Created new API status"));
        }
        deviceStatus.apis_status[api_name].count_sends = count_sends;
        deviceStatus.apis_status[api_name].count_sends_success = count_sends_success;
        deviceStatus.apis_status[api_name].last_send_time = last_send_time_t;
        deviceStatus.apis_status[api_name].is_ok = is_ok;
        debug_outln_info(F("API status updated"));
    }
    
    bool isTimeToSend() const {
        return (millis() - last_send_time > timeout);
    }

    bool lastSendWasOk() const {
        return is_ok;
    }

    void send(JsonDocument &data) {
        _send(data);
        updateSendTime();
    };
};

#endif  // API_