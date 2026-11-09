#include "ags3871_sensor.h"

#include "sensor_names.h"
#include "../utils.h"
#include "drivers/i2c.h"

// The datasheet requires at least 120 seconds before CO concentration data
// should be treated as valid after power-up.
#define AGS3871_WARMUP_MS 120000UL

AGS3871Sensor::AGS3871Sensor(unsigned long sending_timeout)
    : Sensor(sending_timeout)
{
    timeout = sending_timeout;
    sensor_name = AGS3871_SENSOR_NAME;
}

bool AGS3871Sensor::begin()
{
    debug_outln_info(F("Begin AGS3871Sensor"));
    debug_outln_info(F("AGS3871 I2C address: 0x"), String(AGS3871Driver::ADDRESS, HEX));

    I2cBusLock bus;
    if (!bus.ok())
    {
        debug_outln_error(F("AGS3871 I2C bus lock failed"));
        return false;
    }

    // For bring-up, begin() intentionally only checks that the module ACKs on
    // its datasheet address. Register reads happen later in _fetch().
    initialized = ags3871.begin();
    if (initialized)
    {
        // Start the warm-up timer only after the sensor has been detected.
        warmup_started_at_ms = millis();
        last_fetch_time = millis() - timeout;
        debug_outln_info(F("AGS3871 found on I2C"));
    }
    else
    {
        debug_outln_error(F("AGS3871 not found on I2C"));
        debug_outln_info(F("AGS3871 last I2C error: "), String(ags3871.lastI2CError()));
    }

    return initialized;
}

void AGS3871Sensor::_fetch(JsonDocument &data)
{
    if (!initialized)
    {
        return;
    }

    // AGS3871 may ACK on I2C before its gas measurement is usable, so skip
    // concentration reads until the datasheet warm-up interval has elapsed.
    const unsigned long warmup_elapsed_ms = millis() - warmup_started_at_ms;
    if (warmup_elapsed_ms < AGS3871_WARMUP_MS)
    {
        debug_outln_verbose(
            F("AGS3871 warming up, seconds left: "),
            String((AGS3871_WARMUP_MS - warmup_elapsed_ms + 999UL) / 1000UL));
        addValueToJSON(data, F("status"), String(F("warmup")), F("Status"), F(""));
        return;
    }

    debug_outln_verbose(F("fetch AGS3871"));

    I2cBusLock bus;
    if (!bus.ok())
    {
        debug_outln_error(F("AGS3871 I2C bus lock failed in fetch"));
        addValueToJSON(data, F("status"), String(F("i2c_error")), F("Status"), F(""));
        return;
    }

    AGS3871ConcentrationData reading;
    const AGS3871Error err = ags3871.readConcentration(reading);

    if (err == AGS3871_ERROR_NOT_READY)
    {
        // RDY is inverted in the datasheet: status bit 0 set means there is no
        // fresh concentration value yet.
        debug_outln_verbose(F("AGS3871 data not ready, status: "), String(reading.status, HEX));
        addValueToJSON(data, F("status"), String(F("not_ready")), F("Status"), F(""));
        return;
    }

    if (err != AGS3871_OK)
    {
        debug_outln_error(F("AGS3871 concentration read failed"));
        debug_outln_info(F("AGS3871 error code: "), String(err));
        if (err == AGS3871_ERROR_I2C)
        {
            debug_outln_info(F("AGS3871 last I2C error: "), String(ags3871.lastI2CError()));
        }
        addValueToJSON(data, F("status"), String(F("error")), F("Status"), F(""));
        return;
    }

    last_co_ppm = reading.ppm;
    debug_outln_verbose(F("AGS3871 CO [ppm]: "), String(last_co_ppm));
    // Publish only values that passed the driver's I2C, CRC and RDY checks.
    addValueToJSON(data, F("co"), static_cast<float>(last_co_ppm), F("CO"), F("ppm"));
    // UI-only field: Robonomics formatter does not map "status" to datalog.
    addValueToJSON(data, F("status"), String(F("valid")), F("Status"), F(""));
}
