#ifdef ALTRUIST_INSIDE

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
    void _fetch(JsonDocument &data) override;
    void _fetch_one_sensor(JsonDocument &data, HTTPClient& http, const String &ip_address);
    std::vector<String> sensor_addresses;
};

#endif // __HTTP_ALTRUIST_H__

#endif