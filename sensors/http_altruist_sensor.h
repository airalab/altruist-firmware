#ifdef ALTRUIST_INSIGHT

#ifndef __HTTP_ALTRUIST_H__
#define __HTTP_ALTRUIST_H__

#include "sensor.h"
#include "HTTPClient.h"

#define JSON_DATA_PATH "/data.json"
#define SENSOR_URL_PREFIX "http://"

class HTTPAltruistSensor : public Sensor
{

public:
    HTTPAltruistSensor(unsigned long sending_timeout = 1000UL);
    bool begin() override;

private:
    bool _discoverSensors();
    void _fetch(JsonDocument &data) override;
    void _fetch_one_sensor(JsonDocument &data, HTTPClient& http, const String &ip_address);
    std::vector<String> sensor_addresses;
    String chosen_address;
    unsigned long last_success_time = 0;
    uint8_t       consecutive_failures = 0;
    // Discovery retry bookkeeping: when Urban is not initially present,
    // we perform a limited number of rediscovery attempts spaced in time.
    unsigned long last_discovery_attempt_time = 0;
    uint8_t       discovery_attempts = 0;
};

#endif // __HTTP_ALTRUIST_H__

#endif