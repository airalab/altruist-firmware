#ifdef ALTRUIST_INSIDE

#include "http_altruist_sensor.h"
#include "../utils.h"
#include "../intl.h"
#include "sensor_names.h"
#include <ESPmDNS.h>

#define HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT 300000UL

HTTPAltruistSensor::HTTPAltruistSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    if (sending_timeout > HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT) {
    timeout = sending_timeout;
    } else {
    timeout = HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT;
    }
    sensor_name = HTTP_ALTRUIST_SENSOR_NAME;
}

bool HTTPAltruistSensor::begin() {
    debug_outln_info(F("Begin HTTPAltruistSensor"));
    if(mdns_init()!= ESP_OK){
        debug_outln_info(F("mDNS failed to start"));
        return false;
    }
    debug_outln_info(F("mDNS init finished"));

    int nrOfServices = MDNS.queryService("altruist", "tcp");
   
    if (nrOfServices == 0) {
        debug_outln_info(F("No services were found."));
        return false;
    } 
    debug_outln_info(F("Number of services found: "), nrOfServices);

    for (int i = 0; i < nrOfServices; i=i+1) {
        debug_outln_info(F("---------------"));
        debug_outln_info(F("Hostname: "), MDNS.hostname(i));
        debug_outln_info(F("IP address: "), MDNS.address(i).toString());
        debug_outln_info(F("Port: "), MDNS.port(i));
        debug_outln_info(F("---------------"));
    }
    sensor_ip_address = MDNS.address(0).toString();
    sensor_url += MDNS.address(0).toString();
    sensor_url += JSON_DATA_PATH;
    debug_outln_info(F("Http Altruis Sensor started with fetch interval (sec): "), String(timeout/1000));
    last_fetch_time = millis() - timeout;
    return true;
}

void HTTPAltruistSensor::_fetch(JsonDocument &data) {
    debug_outln_info(F("fetch HTTP Altruist"));
    HTTPClient http;
    http.begin(sensor_url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        debug_outln_info(F("Success request to Altruis Urban"));
        addValueToJSON(data, F("IP_address"), sensor_ip_address, INTL_IP_ADDRESS, "");
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            debug_outln_info(F("JSON parse error: "), err.c_str());
            http.end();
            return;
        }
        JsonArray values = doc["sensordatavalues"];
        for (JsonObject v : values) {
            String type = v["value_type"];
            const char* name = type.c_str();
            float value = v["value"].as<float>();

            // Пример соответствия intl_name и units
            String units;
            const char* intl_name;
            if (type == "SDS_P1") {
                intl_name = "PM10";
                units = F("µg/m³");
            } else if (type == "SDS_P2") {
                intl_name = "PM2.5";
                units = F("µg/m³");
            } else if (type == "BME280_temperature") {
                intl_name = INTL_TEMPERATURE;
                units = F("°C");
            } else if (type == "BME280_humidity") {
                intl_name = INTL_HUMIDITY;
                units = F("%");
            } else if (type == "BME280_pressure") {
                intl_name = INTL_PRESSURE;
                units = F("Pa");
            } else if (type == "PCBA_noiseMax") {
                intl_name = INTL_NOISE_MAX;
                units = F("db");
            } else if (type == "PCBA_noiseAvg") {
                intl_name = INTL_NOISE_MEAN;
                units = F("db");
            } else {
                intl_name = type.c_str();  // по умолчанию просто тип
                units = "";
            }

            addValueToJSON(data, type, value, intl_name, units);
        }
        serializeJson(data, Serial);
    } else {
        debug_outln_info(F("Request to Altruist Urban failed, code: "), httpCode);
    }
}

#endif