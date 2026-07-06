#include "tiny_gps_sensor.h"
#include "../utils.h"
#include "../intl.h"
#include "sensor_names.h"

// HardwareSerial Serial0;

GPSSensor::GPSSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    timeout = sending_timeout;
    sensor_name = GPS_SENSOR_NAME;
}

bool GPSSensor::begin() {
    debug_outln_info(F("Trying GPS on 16, 17"));
    serialSDS.begin(9600, SERIAL_8N1, 4, 5);
    while (serialSDS.available() > 0)
        if (gps.encode(serialSDS.read()))
        displayInfo();
    // if (res) {
    //     debug_outln_info(F("RadSens Sensor started with fetch interval (sec): "), String(timeout/1000));
    // }
    return true;
}

void GPSSensor::_fetch(JsonDocument &data) {
    debug_outln_info(F("GPS Sensor fetch"));
    serialSDS.begin(9600, SERIAL_8N1, 4, 5);
    while (serialSDS.available() > 0)
        if (gps.encode(serialSDS.read()))
        displayInfo();
    
    // debug_outln_info(F("radiation "), last_value_gc);
    addValueToJSON(data, F("latitude"), gps.location.lat(), "Latitude", F(""));
    addValueToJSON(data, F("longitude"), gps.location.lng(), "Longitude", F(""));
#if defined(ALTRUIST_BUILD_DEBUG)
    serializeJson(data, Serial);
#endif
}

void GPSSensor::displayInfo()
{
#if defined(ALTRUIST_BUILD_DEBUG)
    Serial.print(F("Location: ")); 
    if (gps.location.isValid())
    {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    }
    else
    {
    Serial.print(F("INVALID"));
    }

    Serial.print(F("  Date/Time: "));
    if (gps.date.isValid())
    {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
    }
    else
    {
    Serial.print(F("INVALID"));
    }

    Serial.print(F(" "));
    if (gps.time.isValid())
    {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
    }
    else
    {
    Serial.print(F("INVALID"));
    }

    Serial.println();
#endif
}
