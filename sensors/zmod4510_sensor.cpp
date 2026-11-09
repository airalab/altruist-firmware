#include "zmod4510_sensor.h"
#include "zmod4510_config_no2_o3.h"

#include "sensor_names.h"
#include "../utils.h"
#include "../intl.h"
#include "drivers/i2c.h"

// Renesas NO2_O3 mode requires a 6 s sampling cadence
#define ZMOD4510_SENSOR_FETCH_INTERVAL_MS 6000UL

ZMOD4510Sensor::ZMOD4510Sensor(unsigned long sending_timeout)
    : Sensor(sending_timeout)
{
    (void)sending_timeout;
    timeout = ZMOD4510_SENSOR_FETCH_INTERVAL_MS;
    sensor_name = ZMOD4510_SENSOR_NAME;
}

bool ZMOD4510Sensor::begin()
{
    debug_outln_info(F("Begin ZMOD4510Sensor"));

    I2cBusLock bus;
    if (!bus.ok())
    {
        debug_outln_error(F("ZMOD4510 I2C bus lock failed"));
        return false;
    }

    if (zmod4510_fill_interface(&hal) != 0)
    {
        debug_outln_error(F("ZMOD4510 HAL init failed"));
        return false;
    }

    // Bind sensor-specific configuration tables from the Renesas NO2_O3 package
    dev.i2c_addr = ZMOD4510_I2C_ADDR;
    dev.pid = ZMOD4510_PID;
    dev.init_conf = &zmod_no2_o3_sensor_cfg[INIT];
    dev.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT];
    dev.prod_data = prod_data;

    // Attach platform HAL callbacks and verify that the sensor responds on I2C
    last_error = zmod4xxx_init(&dev, &hal);
    if (last_error != ZMOD4XXX_OK)
    {
        debug_outln_error(F("zmod4xxx_init failed"));
        debug_outln_info(F("zmod4xxx_init error code: "), String(last_error));
        return false;
    }

    // Read calibration/configuration data from the module and prepare measurement mode
    last_error = zmod4xxx_read_sensor_info(&dev);
    if (last_error != ZMOD4XXX_OK)
    {
        debug_outln_error(F("zmod4xxx_read_sensor_info failed: "));
        debug_outln_info(F("zmod4xxx_read_sensor_info error code: "), String(last_error));
        return false;
    }

    last_error = zmod4xxx_prepare_sensor(&dev);
    if (last_error != ZMOD4XXX_OK)
    {
        debug_outln_error(F("zmod4xxx_prepare_sensor failed: "));
        debug_outln_info(F("zmod4xxx_prepare_sensor error code: "), String(last_error));
        return false;
    }

    // Initialize the Renesas NO2/O3 algorithm state once at startup
    last_error = init_no2_o3(&algo_handle);
    if (last_error != NO2_O3_OK)
    {
        debug_outln_error(F("init_no2_o3 failed"));
        debug_outln_info(F("init_no2_o3 error code: "), String(last_error));
        return false;
    }

    initialized = true;
    last_fetch_time = millis() - timeout;

    debug_outln_info(F("ZMOD4510 started with fetch interval (sec): "), String(timeout / 1000));

    return true;
}

void ZMOD4510Sensor::_fetch(JsonDocument &data)
{
    (void)data;

    if (!initialized)
    {
        return;
    }

    debug_outln_verbose(F("fetch ZMOD4510"));

    I2cBusLock bus;
    if (!bus.ok())
    {
        debug_outln_error(F("ZMOD4510 I2C bus lock failed in fetch"));
        return;
    }

    // Start one full NO2_O3 measurement cycle in the sensor sequencer
    int ret = zmod4xxx_start_measurement(&dev);
    if (ret != ZMOD4XXX_OK)
    {
        debug_outln_error(F("ZMOD4510 start_measurement failed"));
        debug_outln_info(F("ZMOD4510 start_measurement error code: "), String(ret));
        return;
    }

    // The Renesas NO2_O3 algorithm expects a fixed 6 s sampling cadence
    // Reading earlier would break the measurement sequence and algorithm timing
    dev.delay_ms(ZMOD4510_NO2_O3_SAMPLE_TIME);

    // Verify that the internal measurement sequencer has finished before reading ADC data
    ret = zmod4xxx_read_status(&dev, &status);
    if (ret != ZMOD4XXX_OK)
    {
        debug_outln_error(F("ZMOD4510 read_status failed"));
        debug_outln_info(F("ZMOD4510 read_status error code: "), String(ret));
        return;
    }

    // If the sequencer is still running, ADC results are not yet valid
    if (status & STATUS_SEQUENCER_RUNNING_MASK)
    {
        ret = zmod4xxx_check_error_event(&dev);
        switch (ret)
        {
        case ERROR_POR_EVENT:
            debug_outln_error(F("ZMOD4510 unexpected sensor reset"));
            debug_outln_info(F("ZMOD4510 error code: "), String(ret));
            break;
        case ZMOD4XXX_OK:
            debug_outln_error(F("ZMOD4510 measurement still running, wrong sensor setup"));
            break;
        default:
            debug_outln_error(F("ZMOD4510 measurement still running, unknown error"));
            debug_outln_info(F("ZMOD4510 error code: "), String(ret));
            break;
        }
        return;
    }

    // Read raw ADC output from the sensor; gas concentrations are computed later by the algorithm
    ret = zmod4xxx_read_adc_result(&dev, adc_result);
    if (ret != ZMOD4XXX_OK)
    {
        debug_outln_error(F("ZMOD4510 read_adc_result failed"));
        debug_outln_info(F("ZMOD4510 read_adc_result error code: "), String(ret));
        return;
    }

    algo_input.adc_result = adc_result;

    // Default Renesas fallback values:
    // RH = 50%, temperature = -300°C enables on-chip temperature usage
    algo_input.humidity_pct = default_humidity;
    algo_input.temperature_degc = default_temperature;

    // Prefer ambient temperature/humidity from BME280 when available
    JsonObject bme = data[BME_SENSOR_NAME];
    if (!bme.isNull())
    {
        JsonObject bme_humidity = bme["humidity"];
        if (!bme_humidity.isNull() && bme_humidity["value"].is<float>())
        {
            float humidity = bme_humidity["value"].as<float>();
            if (humidity >= 0.0f && humidity <= 100.0f)
            {
                algo_input.humidity_pct = humidity;
            }
        }

        JsonObject bme_temperature = bme["temperature"];
        if (!bme_temperature.isNull() && bme_temperature["value"].is<float>())
        {
            float temperature = bme_temperature["value"].as<float>();
            if (temperature > -40.0f && temperature < 85.0f)
            {
                algo_input.temperature_degc = temperature;
            }
        }
    }

    debug_outln_verbose(
        F("ZMOD4510 compensation T/RH: "),
        String(algo_input.temperature_degc, 2) + F(" C, ") + String(algo_input.humidity_pct, 2) + F(" %"));

    // Convert raw sensor data into O3, NO2 and AQI values using the Renesas NO2_O3 library
    ret = calc_no2_o3(&algo_handle, &dev, &algo_input, &algo_results);

    last_o3_value = algo_results.O3_conc_ppb;
    last_no2_value = algo_results.NO2_conc_ppb;
    last_fast_aqi_value = algo_results.FAST_AQI;
    last_epa_aqi_value = algo_results.EPA_AQI;

    debug_outln_verbose(F("ZMOD4510 O3 [ppb]: "), String(last_o3_value, 2));
    debug_outln_verbose(F("ZMOD4510 NO2 [ppb]: "), String(last_no2_value, 2));
    debug_outln_verbose(F("ZMOD4510 FAST_AQI: "), String(last_fast_aqi_value, 0));
    debug_outln_verbose(F("ZMOD4510 EPA_AQI: "), String(last_epa_aqi_value, 0));

    // The algorithm return code indicates result validity: warm-up, valid, or damage state
    JsonObject sensorObj = data[sensor_name];
    String sensor_status;
    switch (ret)
    {
    case NO2_O3_STABILIZATION:
        debug_outln_verbose(F("ZMOD4510 status: Warm-Up"));
        if (!sensorObj.isNull())
        {
            sensorObj.remove("o3");
            sensorObj.remove("no2");
            sensorObj.remove("fast_aqi");
            sensorObj.remove("epa_aqi");
        }
        sensor_status = F("warmup");
        break;
    case NO2_O3_OK:
        debug_outln_verbose(F("ZMOD4510 status: Valid"));
        addValueToJSON(data, F("o3"), last_o3_value, F("O3"), F("ppb"));
        addValueToJSON(data, F("no2"), last_no2_value, F("NO2"), F("ppb"));
        addValueToJSON(data, F("fast_aqi"), last_fast_aqi_value, F("FAST AQI"), F(""));
        addValueToJSON(data, F("epa_aqi"), last_epa_aqi_value, F("EPA AQI"), F(""));
        sensor_status = F("valid");
        break;
    case NO2_O3_DAMAGE:
        debug_outln_error(F("ZMOD4510 status: Damage"));
        if (!sensorObj.isNull())
        {
            sensorObj.remove("o3");
            sensorObj.remove("no2");
            sensorObj.remove("fast_aqi");
            sensorObj.remove("epa_aqi");
        }
        sensor_status = F("damage");
        break;
    default:
        debug_outln_error(F("ZMOD4510 algorithm calculation failed"));
        debug_outln_info(F("ZMOD4510 algorithm error code: "), String(ret));
        if (!sensorObj.isNull())
        {
            sensorObj.remove("o3");
            sensorObj.remove("no2");
            sensorObj.remove("fast_aqi");
            sensorObj.remove("epa_aqi");
        }
        sensor_status = F("error");
    }

    addValueToJSON(data, F("status"), sensor_status, F("Status"), F(""));
}
