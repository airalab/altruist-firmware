#ifdef ALTRUIST_INSIDE

#ifndef __HTTP_ALTRUIST_H__
#define __HTTP_ALTRUIST_H__

#include "sensor.h"
#include "HTTPClient.h"

#define JSON_DATA_PATH "/data.json"

class HTTPAltruistSensor : public Sensor
{

public:
    HTTPAltruistSensor(unsigned long sending_timeout = 1000UL);

    bool begin() override;

private:
    void _fetch(JsonDocument &data) override;
    String sensor_url = "http://";
};

#endif // __HTTP_ALTRUIST_H__

#endif