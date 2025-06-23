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

String supported_sensor_names[] = {
  SDS_SENSOR_NAME, 
  BME_SENSOR_NAME, 
  I2S_NOISE_SENSOR_NAME, 
  SCD4X_SENSOR_NAME, 
  RADSENS_SENSOR_NAME
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
  }
  return nullptr;
}

#endif  // SENSOR_FACTORY_H