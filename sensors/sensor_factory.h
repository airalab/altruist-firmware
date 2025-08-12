#ifndef SENSOR_FACTORY_H
#define SENSOR_FACTORY_H

#include "sensor.h"
#include "sensor_names.h"
#include "bmx280i2c_sensor.h"
#include "sds011_sensor.h"
#include "i2s_noise_sensor.h"
#include "radsens_sensor.h"
#include "tiny_gps_sensor.h"
#include "scd4x_sensor.h"
#if defined(ALTRUIST_INSIDE)
#include "http_altruist_sensor.h"
#include "bmx680i2c_sensor.h"
#endif

String supported_sensor_names[] = {
  BME_SENSOR_NAME, 
#if defined(ALTRUIST_URBAN)
  SDS_SENSOR_NAME,
  I2S_NOISE_SENSOR_NAME,
#endif
  SCD4X_SENSOR_NAME, 
  RADSENS_SENSOR_NAME,
#if defined(ALTRUIST_INSIDE)
  HTTP_ALTRUIST_SENSOR_NAME,
  BME680_SENSOR_NAME,
#endif
};

Sensor* createSensor(const String &sensorType, unsigned long sending_timeout) {
  if (sensorType == SDS_SENSOR_NAME) {
    return new SDS011Sensor(sending_timeout);
  } else if (sensorType == BME_SENSOR_NAME) {
    return new BMX280Sensor(sending_timeout);
  } else if (sensorType == I2S_NOISE_SENSOR_NAME) {
    return new I2SNoiseSensor(sending_timeout);
  } else if (sensorType == RADSENS_SENSOR_NAME) {
    return new RadSensSensor(sending_timeout);
  } else if (sensorType == GPS_SENSOR_NAME) {
    return new GPSSensor(sending_timeout);
  } else if (sensorType == SCD4X_SENSOR_NAME) {
    return new SCD4xSensor(sending_timeout);
#if defined(ALTRUIST_INSIDE)
  } else if (sensorType == HTTP_ALTRUIST_SENSOR_NAME) {
    return new HTTPAltruistSensor(sending_timeout);
  } else if (sensorType == BME680_SENSOR_NAME) {
    return new BME680Sensor(sending_timeout);
#endif
  }
  return nullptr;
}

#endif  // SENSOR_FACTORY_H