#ifndef __I2SNOISE_H__
#define __I2SNOISE_H__

#include "sensor.h"

class I2SNoiseSensor : public Sensor
{

public:
    I2SNoiseSensor(unsigned long sending_timeout = 1000UL);

    bool begin() override;
    void setSDSRunning(bool running) { is_SDS_running = running; }

private:
#if defined(ALTRUIST_BUILD_DEBUG)
    uart_v2::Noise debugNoiseState;
#endif
    void _fetch(JsonDocument &data) override;
    bool is_SDS_running = false;
    uint8_t last_value_DBMETER = 0;
    uint8_t last_value_DBMETER_max = 0;
    uint32_t last_value_DBMETER_sum = 0;
    uint32_t last_value_DBMETER_count = 0;
    float last_value_DBMETER_mean = 0;
    unsigned long last_send_time = 0;
    void reset_values();
};

#endif // __I2SNOISE_H__