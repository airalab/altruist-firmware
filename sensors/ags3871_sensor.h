#ifndef __AGS3871_SENSOR_H__
#define __AGS3871_SENSOR_H__

#include "sensor.h"
#include "drivers/ags3871/ags3871.h"

class AGS3871Sensor : public Sensor
{

public:
    AGS3871Sensor(unsigned long sending_timeout = 1000UL);
    bool begin() override;

private:
    void _fetch(JsonDocument &data) override;

    // Low-level AGS3871 protocol driver. The wrapper owns sensor lifecycle,
    // timing and JSON integration; the driver only talks to the I2C device.
    AGS3871Driver ags3871;

    // AGS3871 needs at least 120 s warm-up before concentration data is valid.
    unsigned long warmup_started_at_ms = 0;

    // Last valid CO concentration in ppm. Kept so later fetch logic can expose
    // or compare the latest stable value without re-reading the sensor state.
    uint32_t last_co_ppm = 0;

    // Guards _fetch() until begin() has confirmed that the sensor ACKs on I2C.
    bool initialized = false;
};

#endif // __AGS3871_SENSOR_H__
