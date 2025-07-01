#ifndef __BME680_H__
#define __BME680_H__

#include "bme68x.h"
#include "driver/i2c.h"

#define BME68X_DEFAULT_ADDRESS (0x77)

struct bme680_i2c_intf {
    i2c_port_t i2c_num;
    uint8_t address;
};

class Adafruit_BME680 {
public:
  static constexpr int reading_not_started = -1;
  static constexpr int reading_complete = 0;

  Adafruit_BME680(uint8_t i2c_num, uint8_t addr = BME68X_DEFAULT_ADDRESS);

  bool begin(bool initSettings = true);
  float readTemperature();
  float readPressure();
  float readHumidity();
  uint32_t readGas();
  float readAltitude(float seaLevel);
  bool setTemperatureOversampling(uint8_t os);
  bool setPressureOversampling(uint8_t os);
  bool setHumidityOversampling(uint8_t os);
  bool setIIRFilterSize(uint8_t fs);
  bool setGasHeater(uint16_t heaterTemp, uint16_t heaterTime);
  bool setODR(uint8_t odr);
  bool performReading();
  uint32_t beginReading();
  bool endReading();
  int remainingReadingMillis();

  float temperature;
  uint32_t pressure;
  float humidity;
  uint32_t gas_resistance;

private:
  uint8_t _i2c_num;
  uint8_t _i2c_addr;
  bme680_i2c_intf _i2c_intf;

  int32_t _sensorID;
  uint32_t _meas_start = 0;
  uint16_t _meas_period = 0;

  struct bme68x_dev gas_sensor;
  struct bme68x_conf gas_conf;
  struct bme68x_heatr_conf gas_heatr_conf;
};

#endif
