/*!
 * @file Adafruit_BME680.cpp
 *
 * @mainpage Adafruit BME680 temperature, humidity, barometric pressure and gas
 * sensor driver
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for Adafruit's BME680 driver for the
 * Arduino platform.  It is designed specifically to work with the
 * Adafruit BME680 breakout: https://www.adafruit.com/products/3660
 *
 * These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
 * to interface with the breakout.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#include "bme680.h"
#include "driver/i2c.h"
#include <esp_log.h>
#include <Arduino.h>
#include <cstring> // Required for memcpy

#define TAG "BME680"


static int8_t i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    auto *i2c = (bme680_i2c_intf *)intf_ptr;

    // Write register address
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c->i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return -1;

    // Read data
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, reg_data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, reg_data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c->i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? 0 : -1;
}

static int8_t i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    auto *i2c = (bme680_i2c_intf *)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c->address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, reg_data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c->i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? 0 : -1;
}


static void delay_usec(uint32_t us, void *intf_ptr) {
  (void)intf_ptr; // Unused parameter
  delayMicroseconds(us);
  yield();
}

Adafruit_BME680::Adafruit_BME680(uint8_t i2c_num, uint8_t addr)
    : _i2c_num(i2c_num), _i2c_addr(addr) {}

bool Adafruit_BME680::begin(bool initSettings) {
    _i2c_intf.i2c_num = static_cast<i2c_port_t>(_i2c_num);;
    _i2c_intf.address = _i2c_addr;
  gas_sensor.chip_id = _i2c_addr;
  gas_sensor.intf = BME68X_I2C_INTF;
  gas_sensor.intf_ptr = &_i2c_intf;
  gas_sensor.read = &i2c_read;
  gas_sensor.write = &i2c_write;
  gas_sensor.delay_us = delay_usec;
  gas_sensor.amb_temp = 25;

  if (bme68x_init(&gas_sensor) != BME68X_OK) {
    ESP_LOGE(TAG, "Failed to initialize BME680");
    return false;
  }

  if (initSettings) {
    setIIRFilterSize(BME68X_FILTER_SIZE_3);
    setODR(BME68X_ODR_NONE);
    setHumidityOversampling(BME68X_OS_2X);
    setPressureOversampling(BME68X_OS_4X);
    setTemperatureOversampling(BME68X_OS_8X);
    setGasHeater(320, 150); // 320*C for 150 ms
  } else {
    setGasHeater(0, 0);
  }

  return bme68x_set_op_mode(BME68X_FORCED_MODE, &gas_sensor) == BME68X_OK;
}


/*!
 *  @brief  Performs a reading and returns the ambient temperature.
 *  @return Temperature in degrees Centigrade
 */
float Adafruit_BME680::readTemperature(void) {
  performReading();
  return temperature;
}

/*!
 *  @brief Performs a reading and returns the barometric pressure.
 *  @return Barometic pressure in Pascals
 */
float Adafruit_BME680::readPressure(void) {
  performReading();
  return pressure;
}

/*!
 *  @brief  Performs a reading and returns the relative humidity.
 *  @return Relative humidity as floating point
 */
float Adafruit_BME680::readHumidity(void) {
  performReading();
  return humidity;
}

/*!
 *  @brief Calculates the resistance of the MOX gas sensor.
 *  @return Resistance in Ohms
 */
uint32_t Adafruit_BME680::readGas(void) {
  performReading();
  return gas_resistance;
}

/*!
 *  @brief  Calculates the altitude (in meters).
 *          Reads the current atmostpheric pressure (in hPa) from the sensor and
 * calculates via the provided sea-level pressure (in hPa).
 *  @param  seaLevel
 *          Sea-level pressure in hPa
 *  @return Altitude in meters
 */
float Adafruit_BME680::readAltitude(float seaLevel) {
  // Equation taken from BMP180 datasheet (page 16):
  //  http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

  // Note that using the equation from wikipedia can give bad results
  // at high altitude. See this thread for more information:
  //  http://forums.adafruit.com/viewtopic.php?f=22&t=58064

  float atmospheric = readPressure() / 100.0F;
  return 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.1903));
}

/*!
 *  @brief  Performs a full reading of all 4 sensors in the BME680.
 *          Assigns the internal Adafruit_BME680#temperature,
 * Adafruit_BME680#pressure, Adafruit_BME680#humidity and
 * Adafruit_BME680#gas_resistance member variables
 *  @return True on success, False on failure
 */
bool Adafruit_BME680::performReading(void) { return endReading(); }

/*! @brief Begin an asynchronous reading.
 *  @return When the reading would be ready as absolute time in millis().
 */
uint32_t Adafruit_BME680::beginReading(void) {
  if (_meas_start != 0) {
    /* A measurement is already in progress */
    return _meas_start + _meas_period;
  }

  int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("Opmode Result: "));
  Serial.println(rslt);
#endif
  if (rslt != BME68X_OK)
    return false;

  /* Calculate delay period in microseconds */
  uint32_t delayus_period = (uint32_t)bme68x_get_meas_dur(
                                BME68X_FORCED_MODE, &gas_conf, &gas_sensor) +
                            ((uint32_t)gas_heatr_conf.heatr_dur * 1000);
  // Serial.print("measure: ");
  // Serial.println(bme68x_get_meas_dur(BME68X_FORCED_MODE, &gas_conf,
  // &gas_sensor)); Serial.print("heater: ");
  // Serial.println((uint32_t)gas_heatr_conf.heatr_dur * 1000);

  _meas_start = millis();
  _meas_period = delayus_period / 1000;

  return _meas_start + _meas_period;
}

/*! @brief  End an asynchronous reading.
 *          If the asynchronous reading is still in progress, block until it
 * ends. If no asynchronous reading has started, this is equivalent to
 * performReading().
 *  @return Whether success.
 */
bool Adafruit_BME680::endReading(void) {
  uint32_t meas_end = beginReading();

  if (meas_end == 0) {
    return false;
  }

  int remaining_millis = remainingReadingMillis();

  if (remaining_millis > 0) {
#ifdef BME680_DEBUG
    Serial.print(F("Waiting (ms) "));
    Serial.println(remaining_millis);
#endif
    delay(static_cast<unsigned int>(remaining_millis) *
          2); /* Delay till the measurement is ready */
  }
  _meas_start = 0; /* Allow new measurement to begin */
  _meas_period = 0;

#ifdef BME680_DEBUG
  Serial.print(F("t_fine = "));
  Serial.println(gas_sensor.calib.t_fine);
#endif

  struct bme68x_data data;
  uint8_t n_fields;

#ifdef BME680_DEBUG
  Serial.println(F("Getting sensor data"));
#endif

  int8_t rslt =
      bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("GetData Result: "));
  Serial.println(rslt);
#endif
  if (rslt != BME68X_OK)
    return false;

  if (n_fields) {
    temperature = data.temperature;
    humidity = data.humidity;
    pressure = data.pressure;

#ifdef BME680_DEBUG
    Serial.print(F("data.status 0x"));
    Serial.println(data.status, HEX);
#endif
    if (data.status & (BME68X_HEAT_STAB_MSK | BME68X_GASM_VALID_MSK)) {
      // Serial.print("Gas resistance: "); Serial.println(data.gas_resistance);
      gas_resistance = data.gas_resistance;
    } else {
      gas_resistance = 0;
      // Serial.println("Gas reading unstable!");
    }
  }

  return true;
}

/*! @brief  Get remaining time for an asynchronous reading.
 *          If the asynchronous reading is still in progress, how many millis
 * until its completion. If the asynchronous reading is completed, 0. If no
 * asynchronous reading has started, -1 or
 * Adafruit_BME680::reading_not_started. Does not block.
 *  @return Remaining millis until endReading will not block if invoked.
 */
int Adafruit_BME680::remainingReadingMillis(void) {
  if (_meas_start != 0) {
    /* A measurement is already in progress */
    int remaining_time = (int)_meas_period - (millis() - _meas_start);
    return remaining_time < 0 ? reading_complete : remaining_time;
  }
  return reading_not_started;
}

/*!
 *  @brief  Enable and configure gas reading + heater
 *  @param  heaterTemp
 *          Desired temperature in degrees Centigrade
 *  @param  heaterTime
 *          Time to keep heater on in milliseconds
 *  @return True on success, False on failure
 */
bool Adafruit_BME680::setGasHeater(uint16_t heaterTemp, uint16_t heaterTime) {

  if ((heaterTemp == 0) || (heaterTime == 0)) {
    gas_heatr_conf.enable = BME68X_DISABLE;
  } else {
    gas_heatr_conf.enable = BME68X_ENABLE;
    gas_heatr_conf.heatr_temp = heaterTemp;
    gas_heatr_conf.heatr_dur = heaterTime;
  }

  int8_t rslt =
      bme68x_set_heatr_conf(BME68X_FORCED_MODE, &gas_heatr_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetHeaterConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Output Data Rate
 *  @param  odr
 *          Output data rate setting, can be BME68X_ODR_NONE,
 * BME68X_ODR_0_59_MS, BME68X_ODR_10_MS, BME68X_ODR_20_MS, BME68X_ODR_62_5_MS,
 * BME68X_ODR_125_MS, BME68X_ODR_250_MS, BME68X_ODR_500_MS, BME68X_ODR_1000_MS
 *  @return True on success, False on failure
 */

bool Adafruit_BME680::setODR(uint8_t odr) {
  if (odr > BME68X_ODR_NONE)
    return false;

  gas_conf.odr = odr;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Temperature oversampling
 *  @param  oversample
 *          Oversampling setting, can be BME68X_OS_NONE (turn off Temperature
 * reading), BME68X_OS_1X, BME68X_OS_2X, BME68X_OS_4X, BME68X_OS_8X or
 * BME68X_OS_16X
 *  @return True on success, False on failure
 */

bool Adafruit_BME680::setTemperatureOversampling(uint8_t oversample) {
  if (oversample > BME68X_OS_16X)
    return false;

  gas_conf.os_temp = oversample;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Humidity oversampling
 *  @param  oversample
 *          Oversampling setting, can be BME68X_OS_NONE (turn off Humidity
 * reading), BME68X_OS_1X, BME68X_OS_2X, BME68X_OS_4X, BME68X_OS_8X or
 * BME68X_OS_16X
 *  @return True on success, False on failure
 */
bool Adafruit_BME680::setHumidityOversampling(uint8_t oversample) {
  if (oversample > BME68X_OS_16X)
    return false;

  gas_conf.os_hum = oversample;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Pressure oversampling
 *  @param  oversample
 *          Oversampling setting, can be BME68X_OS_NONE (turn off Pressure
 * reading), BME68X_OS_1X, BME68X_OS_2X, BME68X_OS_4X, BME68X_OS_8X or
 * BME68X_OS_16X
 *  @return True on success, False on failure
 */
bool Adafruit_BME680::setPressureOversampling(uint8_t oversample) {
  if (oversample > BME68X_OS_16X)
    return false;

  gas_conf.os_pres = oversample;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for IIR filter.
 *  @param  filtersize
 *          Size of the filter (in samples).
 *          Can be BME68X_FILTER_SIZE_0 (no filtering), BME68X_FILTER_SIZE_1,
 * BME68X_FILTER_SIZE_3, BME68X_FILTER_SIZE_7, BME68X_FILTER_SIZE_15,
 * BME68X_FILTER_SIZE_31, BME68X_FILTER_SIZE_63, BME68X_FILTER_SIZE_127
 *  @return True on success, False on failure
 */
bool Adafruit_BME680::setIIRFilterSize(uint8_t filtersize) {
  if (filtersize > BME68X_FILTER_SIZE_127)
    return false;
  gas_conf.filter = filtersize;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}