/*
  This is a library written for the SCD4x family of CO2 sensors
  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/18365

  Written by Paul Clark @ SparkFun Electronics, June 2nd, 2021

  The SCD41 measures CO2 from 400ppm to 5000ppm with an accuracy of +/- 40ppm + 5% of reading

  This library handles the initialization of the SCD4x and outputs
  CO2 levels, relative humidty, and temperature.

  https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.13

  SparkFun code, firmware, and software is released under the MIT License.
  Please see LICENSE.md for more details.
*/

#include "SparkFun_SCD4x_Arduino_Library.h"
#include "driver/i2c.h"

SCD4x::SCD4x(scd4x_sensor_type_e sensorType)
{
  // Constructor
  _sensorType = sensorType;
}

//Initialize the Serial port
bool SCD4x::begin(bool measBegin, bool autoCalibrate, bool skipStopPeriodicMeasurements, bool pollAndSetDeviceType) {

    bool success = true;


    if (!skipStopPeriodicMeasurements) {
        success &= stopPeriodicMeasurement();
    }


    char serialNumber[13];
    success &= getSerialNumber(serialNumber);

#if defined(ALTRUIST_BUILD_DEBUG)
    Serial.print(F("Begin SCD4x serial number: "));
    Serial.println(serialNumber);
#endif

    if (pollAndSetDeviceType) {
        scd4x_sensor_type_e sensorType;
        success &= getFeatureSetVersion(&sensorType);
        setSensorType(sensorType);
    }

    if (autoCalibrate) {
        success &= setAutomaticSelfCalibrationEnabled(true);
        success &= (getAutomaticSelfCalibrationEnabled() == true);
    } else {
        success &= setAutomaticSelfCalibrationEnabled(false);
        success &= (getAutomaticSelfCalibrationEnabled() == false);
    }

    if (measBegin) {
        success &= startPeriodicMeasurement();
    }

    return success;
}

//Calling this function with nothing sets the debug port to Serial
//You can also call it with other streams like Serial1, SerialUSB, etc.
void SCD4x::enableDebugging(Stream &debugPort)
{
  #if SCD4x_ENABLE_DEBUGLOG
  _debugPort = &debugPort;
  _printDebug = true;
  #endif // if SCD4x_ENABLE_DEBUGLOG
}

//Start periodic measurements. See 3.5.1
//signal update interval is 5 seconds.
bool SCD4x::startPeriodicMeasurement(void)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::startPeriodicMeasurement: periodic measurements are already running"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (true); //Maybe this should be false?
  }

  bool success = sendCommand(SCD4x_COMMAND_START_PERIODIC_MEASUREMENT);
  if (success)
    periodicMeasurementsAreRunning = true;
  return (success);
}

//Stop periodic measurements. See 3.5.3
//Stop periodic measurement to change the sensor configuration or to save power.
//Note that the sensor will only respond to other commands after waiting 500 ms after issuing
//the stop_periodic_measurement command.
bool SCD4x::stopPeriodicMeasurement(uint16_t delayMillis) {
    uint8_t command[] = { 
        (uint8_t)(SCD4x_COMMAND_STOP_PERIODIC_MEASUREMENT >> 8),
        (uint8_t)(SCD4x_COMMAND_STOP_PERIODIC_MEASUREMENT & 0xFF) 
    };

    esp_err_t res = i2c_master_write_to_device(I2C_MASTER_NUM, SCD4x_ADDRESS, command, sizeof(command), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    if (res == ESP_OK) {
        periodicMeasurementsAreRunning = false;
        if (delayMillis > 0) vTaskDelay(delayMillis / portTICK_PERIOD_MS);
        return true;
    }
    return false;
}

//Get 9 bytes from SCD4x. See 3.5.2
//Updates global variables with floats
//Returns true if data is read successfully
//Read sensor output. The measurement data can only be read out once per signal update interval as the
//buffer is emptied upon read-out. If no data is available in the buffer, the sensor returns a NACK.
//To avoid a NACK response, the get_data_ready_status can be issued to check data status
//(see chapter 3.8.2 for further details).
bool SCD4x::readMeasurement() {
    if (!getDataReadyStatus()) return false;

    uint8_t command[] = {
        (uint8_t)(SCD4x_COMMAND_READ_MEASUREMENT >> 8),
        (uint8_t)(SCD4x_COMMAND_READ_MEASUREMENT & 0xFF)
    };

    uint8_t data[9];
    
    esp_err_t res = i2c_master_write_read_device(I2C_MASTER_NUM, SCD4x_ADDRESS, command, sizeof(command), data, sizeof(data), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    if (res != ESP_OK) return false;

    bool error = false;
    uint8_t bytesToCrc[2];

    for (int x = 0; x < 9; x++) {
        switch (x) {
            case 0:
            case 1:
                bytesToCrc[x] = data[x];
                break;
            case 3:
            case 4:
                bytesToCrc[x % 3] = data[x];
                break;
            case 6:
            case 7:
                bytesToCrc[x % 3] = data[x];
                break;
            default:
                if (computeCRC8(bytesToCrc, 2) != data[x]) error = true;
                break;
        }
    }

    if (error) return false;

    co2 = (float)((data[0] << 8) | data[1]);
    temperature = -45 + (((float)((data[3] << 8) | data[4])) * 175 / 65536);
    humidity = ((float)((data[6] << 8) | data[7])) * 100 / 65536;

    co2HasBeenReported = humidityHasBeenReported = temperatureHasBeenReported = false;

    return true;
}

//Returns the latest available CO2 level
//If the current level has already been reported, trigger a new read
uint16_t SCD4x::getCO2(void)
{
  if (co2HasBeenReported == true) //Trigger a new read
    readMeasurement();            //Pull in new co2, humidity, and temp into global vars

  co2HasBeenReported = true;

  return (uint16_t)co2; //Cut off decimal as co2 is 0 to 10,000
}

//Returns the latest available humidity
//If the current level has already been reported, trigger a new read
float SCD4x::getHumidity(void)
{
  if (humidityHasBeenReported == true) //Trigger a new read
    readMeasurement();                 //Pull in new co2, humidity, and temp into global vars

  humidityHasBeenReported = true;

  return humidity;
}

//Returns the latest available temperature
//If the current level has already been reported, trigger a new read
float SCD4x::getTemperature(void)
{
  if (temperatureHasBeenReported == true) //Trigger a new read
    readMeasurement();                    //Pull in new co2, humidity, and temp into global vars

  temperatureHasBeenReported = true;

  return temperature;
}

//Set the temperature offset (C). See 3.6.1
//Max command duration: 1ms
//The user can set delayMillis to zero f they want the function to return immediately.
//The temperature offset has no influence on the SCD4x CO2 accuracy.
//Setting the temperature offset of the SCD4x inside the customer device correctly allows the user
//to leverage the RH and T output signal.
bool SCD4x::setTemperatureOffset(float offset, uint16_t delayMillis)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::setTemperatureOffset: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  if (offset < 0)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::setTemperatureOffset: offset must be >= 0C"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }
  if (offset >= 175)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::setTemperatureOffset: offset must be < 175C"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }
  uint16_t offsetWord = (uint16_t)(offset * 65536 / 175); // Toffset [°C] * 2^16 / 175
  bool success = sendCommand(SCD4x_COMMAND_SET_TEMPERATURE_OFFSET, offsetWord);
  if (delayMillis > 0)
    delay(delayMillis);
  return (success);
}

//Get the temperature offset. See 3.6.2
float SCD4x::getTemperatureOffset(void)
{
 if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::getTemperatureOffset: periodic measurements are running. Returning 0.0"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (0.0);
  }

  float offset;
  #if SCD4x_ENABLE_DEBUGLOG
  bool success = 
  #endif // if SCD4x_ENABLE_DEBUGLOG
  getTemperatureOffset(&offset);
  #if SCD4x_ENABLE_DEBUGLOG
  if ((success == false) && (_printDebug == true))
  {
    _debugPort->println(F("SCD4x::getTemperatureOffset: failed to read offset. Returning 0.0"));
  }
  #endif // if SCD4x_ENABLE_DEBUGLOG
  return (offset);
}

//Get the temperature offset. See 3.6.2
bool SCD4x::getTemperatureOffset(float *offset)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::getTemperatureOffset: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  uint16_t offsetWord = 0; // offset will be zero if readRegister fails
  bool success = readRegister(SCD4x_COMMAND_GET_TEMPERATURE_OFFSET, &offsetWord, 1);
  *offset = ((float)offsetWord) * 175.0 / 65535.0;
  return (success);
}

//Set the sensor altitude (metres above sea level). See 3.6.3
//Max command duration: 1ms
//The user can set delayMillis to zero if they want the function to return immediately.
//Reading and writing of the sensor altitude must be done while the SCD4x is in idle mode.
//Typically, the sensor altitude is set once after device installation. To save the setting to the EEPROM,
//the persist setting (see chapter 3.9.1) command must be issued.
//Per default, the sensor altitude is set to 0 meter above sea-level.
bool SCD4x::setSensorAltitude(uint16_t altitude, uint16_t delayMillis)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::setSensorAltitude: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  bool success = sendCommand(SCD4x_COMMAND_SET_SENSOR_ALTITUDE, altitude);
  if (delayMillis > 0)
    delay(delayMillis);
  return (success);
}

//Get the sensor altitude. See 3.6.4
uint16_t SCD4x::getSensorAltitude(void)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::getSensorAltitude: periodic measurements are running. Returning 0"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (0);
  }

  uint16_t altitude = 0;
  #if SCD4x_ENABLE_DEBUGLOG
  bool success = 
  #endif // if SCD4x_ENABLE_DEBUGLOG
  getSensorAltitude(&altitude);
  #if SCD4x_ENABLE_DEBUGLOG
  if ((success == false) && (_printDebug == true))
  {
    _debugPort->println(F("SCD4x::getSensorAltitude: failed to read altitude. Returning 0"));
  }
  #endif // if SCD4x_ENABLE_DEBUGLOG
  return (altitude);
}

//Get the sensor altitude. See 3.6.4
bool SCD4x::getSensorAltitude(uint16_t *altitude)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::getSensorAltitude: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  return (readRegister(SCD4x_COMMAND_GET_SENSOR_ALTITUDE, altitude, 1));
}

//Set the ambient pressure (Pa). See 3.6.5
//Max command duration: 1ms
//The user can set delayMillis to zero if they want the function to return immediately.
//The set_ambient_pressure command can be sent during periodic measurements to enable continuous pressure compensation.
//setAmbientPressure overrides setSensorAltitude
bool SCD4x::setAmbientPressure(float pressure, uint16_t delayMillis)
{
  if (pressure < 0)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::setAmbientPressure: pressure must be >= 0 Pa"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }
  if (pressure > 6553500)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::setAmbientPressure: pressure must be <= 6553500 Pa"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }
  uint16_t pressureWord = (uint16_t)(pressure / 100);
  bool success = sendCommand(SCD4x_COMMAND_SET_AMBIENT_PRESSURE, pressureWord);
  if (delayMillis > 0)
    delay(delayMillis);
  return (success);
}

//Perform forced recalibration. See 3.7.1
float SCD4x::performForcedRecalibration(uint16_t concentration)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::performForcedRecalibration: periodic measurements are running. Returning 0.0"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (0.0);
  }

  float correction = 0.0;
  #if SCD4x_ENABLE_DEBUGLOG
  bool success = 
  #endif // if SCD4x_ENABLE_DEBUGLOG
  performForcedRecalibration(concentration, &correction);
  #if SCD4x_ENABLE_DEBUGLOG
  if ((success == false) && (_printDebug == true))
  {
    _debugPort->println(F("SCD4x::performForcedRecalibration: FRC failed"));
  }
  #endif // if SCD4x_ENABLE_DEBUGLOG
  return (correction);
}

//Perform forced recalibration. See 3.7.1
//To successfully conduct an accurate forced recalibration, the following steps need to be carried out:
//1. Operate the SCD4x in the operation mode later used in normal sensor operation (periodic measurement,
//   low power periodic measurement or single shot) for > 3 minutes in an environment with homogenous and
//   constant CO2 concentration.
//2. Issue stop_periodic_measurement. Wait 500 ms for the stop command to complete.
//3. Subsequently issue the perform_forced_recalibration command and optionally read out the FRC correction
//   (i.e. the magnitude of the correction) after waiting for 400 ms for the command to complete.
//A return value of 0xffff indicates that the forced recalibration has failed.
bool SCD4x::performForcedRecalibration(uint16_t concentration, float *correction)
{
    if (periodicMeasurementsAreRunning)
    {
#if SCD4x_ENABLE_DEBUGLOG
        if (_printDebug == true)
        {
            _debugPort->println(F("SCD4x::performForcedRecalibration: periodic measurements are running. Aborting"));
        }
#endif
        return false;
    }

    uint16_t correctionWord;
    bool success = sendCommand(SCD4x_COMMAND_PERFORM_FORCED_CALIBRATION, concentration);

    if (!success)
        return false;

    vTaskDelay(pdMS_TO_TICKS(400)); // Datasheet specifies this delay

    uint8_t data[3];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SCD4x_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_ACK);  // Read first 2 bytes (correction word)
    i2c_master_read_byte(cmd, &data[2], I2C_MASTER_NACK); // Read CRC
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
#if SCD4x_ENABLE_DEBUGLOG
        if (_printDebug == true)
        {
            _debugPort->println(F("SCD4x::performForcedRecalibration: I2C read error"));
        }
#endif
        return false;
    }

    uint8_t foundCrc = computeCRC8(data, 2);
    if (foundCrc != data[2])
    {
#if SCD4x_ENABLE_DEBUGLOG
        if (_printDebug == true)
        {
            _debugPort->print(F("SCD4x::performForcedRecalibration: CRC error. Expected 0x"));
            _debugPort->print(foundCrc, HEX);
            _debugPort->print(F(", got 0x"));
            _debugPort->println(data[2], HEX);
        }
#endif
        return false;
    }

    correctionWord = ((uint16_t)data[0] << 8) | data[1];
    *correction = ((float)correctionWord) - 32768; // FRC correction [ppm CO2] = word[0] – 0x8000

    if (correctionWord == 0xFFFF) // A return value of 0xFFFF indicates failure
        return false;

    return true;
}

//Enable/disable automatic self calibration. See 3.7.2
//Set the current state (enabled / disabled) of the automatic self-calibration. By default, ASC is enabled.
//To save the setting to the EEPROM, the persist_setting (see chapter 3.9.1) command must be issued.
bool SCD4x::setAutomaticSelfCalibrationEnabled(bool enabled, uint16_t delayMillis)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::setAutomaticSelfCalibrationEnabled: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  uint16_t enabledWord = enabled == true ? 0x0001 : 0x0000;
  bool success = sendCommand(SCD4x_COMMAND_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, enabledWord);
  if (delayMillis > 0)
    delay(delayMillis);
  return (success);
}

//Check if automatic self calibration is enabled. See 3.7.3
bool SCD4x::getAutomaticSelfCalibrationEnabled(void)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::getAutomaticSelfCalibrationEnabled: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  uint16_t enabled;
  bool success = getAutomaticSelfCalibrationEnabled(&enabled);
  if (success == false)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("getAutomaticSelfCalibrationEnabled: failed to get self calibration status. Returning false"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }
  return (enabled == 0x0001);
}

//Check if automatic self calibration is enabled. See 3.7.3
bool SCD4x::getAutomaticSelfCalibrationEnabled(uint16_t *enabled)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::getAutomaticSelfCalibrationEnabled: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  return (readRegister(SCD4x_COMMAND_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED, enabled, 1));
}

//Start low power periodic measurements. See 3.8.1
//Signal update interval will be 30 seconds instead of 5
bool SCD4x::startLowPowerPeriodicMeasurement(void)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::startLowPowerPeriodicMeasurement: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  bool success = sendCommand(SCD4x_COMMAND_START_LOW_POWER_PERIODIC_MEASUREMENT);
  if (success)
    periodicMeasurementsAreRunning = true;
  return (success);
}

//Returns true when data is available. See 3.8.2
bool SCD4x::getDataReadyStatus(void)
{
  uint16_t response;
  bool success = readRegister(SCD4x_COMMAND_GET_DATA_READY_STATUS, &response, 1);

  if (success == false)
    return (false);

  //If the least significant 11 bits of word[0] are 0 → data not ready
  //else → data ready for read-out
  if ((response & 0x07ff) == 0x0000)
    return (false);
  return (true);
}

//Persist settings: copy settings (e.g. temperature offset) from RAM to EEPROM. See 3.9.1
//Configuration settings such as the temperature offset, sensor altitude and the ASC enabled/disabled parameter
//are by default stored in the volatile memory (RAM) only and will be lost after a power-cycle. The persist_settings
//command stores the current configuration in the EEPROM of the SCD4x, making them persistent across power-cycling.
//To avoid unnecessary wear of the EEPROM, the persist_settings command should only be sent when persistence is required
//and if actual changes to the configuration have been made. The EEPROM is guaranteed to endure at least 2000 write
//cycles before failure.
bool SCD4x::persistSettings(uint16_t delayMillis)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::persistSettings: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  bool success = sendCommand(SCD4x_COMMAND_PERSIST_SETTINGS);
  if (delayMillis > 0)
    delay(delayMillis);
  return (success);
}

//Get 9 bytes from SCD4x. Convert 48-bit serial number to ASCII chars. See 3.9.2
//Returns true if serial number is read successfully
//Reading out the serial number can be used to identify the chip and to verify the presence of the sensor.
bool SCD4x::getSerialNumber(char *serialNumber) {
    uint8_t command[] = { 
        (uint8_t)(SCD4x_COMMAND_GET_SERIAL_NUMBER >> 8), 
        (uint8_t)(SCD4x_COMMAND_GET_SERIAL_NUMBER & 0xFF) 
    };

    uint8_t data[9];
    
    esp_err_t res = i2c_master_write_read_device(I2C_MASTER_NUM, SCD4x_ADDRESS, command, sizeof(command), data, sizeof(data), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    if (res != ESP_OK) return false;

    bool error = false;
    uint8_t bytesToCrc[2];
    int digit = 0;

    for (int x = 0; x < 9; x++) {
        switch (x) {
            case 0:
            case 1:
            case 3:
            case 4:
            case 6:
            case 7:
                serialNumber[digit++] = convertHexToASCII(data[x] >> 4);
                serialNumber[digit++] = convertHexToASCII(data[x] & 0x0F);
                bytesToCrc[x % 3] = data[x];
                break;
            default:
                if (computeCRC8(bytesToCrc, 2) != data[x]) error = true;
                break;
        }
    }

    serialNumber[digit] = 0;

    return !error;
}

//PRIVATE: Convert serial number digit to ASCII
char SCD4x::convertHexToASCII(uint8_t digit)
{
  if (digit <= 9)
    return (char(digit + 0x30));
  else
    return (char(digit + 0x41 - 10)); // Use upper case for A-F
}

//Perform self test. Takes 10 seconds to complete. See 3.9.3
//The perform_self_test feature can be used as an end-of-line test to check sensor functionality
//and the customer power supply to the sensor.
bool SCD4x::performSelfTest(void)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::performSelfTest: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  uint16_t response;

  #if SCD4x_ENABLE_DEBUGLOG
  if (_printDebug == true)
    _debugPort->println(F("SCD4x::performSelfTest: delaying for 10 seconds..."));
  #endif // if SCD4x_ENABLE_DEBUGLOG

  bool success = readRegister(SCD4x_COMMAND_PERFORM_SELF_TEST, &response, 10000);

  #if SCD4x_ENABLE_DEBUGLOG
  if (_printDebug == true)
  {
    _debugPort->print(F("SCD4x::performSelfTest: sensor response is 0x"));
    if (response < 0x1000) _debugPort->print(F("0"));
    if (response < 0x100) _debugPort->print(F("0"));
    if (response < 0x10) _debugPort->print(F("0"));
    _debugPort->println(response, HEX);
  }
  #endif // if SCD4x_ENABLE_DEBUGLOG

  return (success && (response == 0x0000)); // word[0] = 0 → no malfunction detected
}

//Peform factory reset. See 3.9.4
//The perform_factory_reset command resets all configuration settings stored in the EEPROM
//and erases the FRC and ASC algorithm history.
bool SCD4x::performFactoryReset(uint16_t delayMillis)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::performFactoryReset: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  bool success = sendCommand(SCD4x_COMMAND_PERFORM_FACTORY_RESET);
  if (delayMillis > 0)
    delay(delayMillis);
  return (success);
}

//Reinit. See 3.9.5
//The reinit command reinitializes the sensor by reloading user settings from EEPROM.
//Before sending the reinit command, the stop measurement command must be issued.
//If the reinit command does not trigger the desired re-initialization,
//a power-cycle should be applied to the SCD4x.
bool SCD4x::reInit(uint16_t delayMillis)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::reInit: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  bool success = sendCommand(SCD4x_COMMAND_REINIT);
  if (delayMillis > 0)
    delay(delayMillis);
  return (success);
}

//Low Power Single Shot. See 3.10.1
//In addition to periodic measurement modes, the SCD41 features a single shot measurement mode,
//i.e. allows for on-demand measurements.
//The typical communication sequence is as follows:
//1. The sensor is powered up.
//2. The I2C master sends a single shot command and waits for the indicated max. command duration time.
//3. The I2C master reads out data with the read measurement sequence (chapter 3.5.2).
//4. Steps 2-3 are repeated as required by the application.
bool SCD4x::measureSingleShot(void)
{
  if (_sensorType != SCD4x_SENSOR_SCD41)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::measureSingleShot: _sensorType is not SCD4x_SENSOR_SCD41"));
      _debugPort->println(F("SCD41's need to be instantiated using: SCD4x mySensor(SCD4x_SENSOR_SCD41)"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return(false);
  }

  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::measureSingleShot: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  bool success = sendCommand(SCD4x_COMMAND_MEASURE_SINGLE_SHOT);

  #if SCD4x_ENABLE_DEBUGLOG
  if (success && (_printDebug == true))
  {
    _debugPort->println(F("SCD4x::measureSingleShot: your data will be ready in five seconds"));
  }
  #endif // if SCD4x_ENABLE_DEBUGLOG

  return (success);
}

//On-demand measurement of relative humidity and temperature only.
//The sensor output is read using the read_measurement command (chapter 3.5.2).
//CO2 output is returned as 0 ppm.
bool SCD4x::measureSingleShotRHTOnly(void)
{
  if (_sensorType != SCD4x_SENSOR_SCD41)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::measureSingleShotRHTOnly: _sensorType is not SCD4x_SENSOR_SCD41"));
      _debugPort->println(F("SCD41's need to be instantiated using: SCD4x mySensor(SCD4x_SENSOR_SCD41)"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return(false);
  }

  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::measureSingleShotRHTOnly: periodic measurements are running. Aborting"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  bool success = sendCommand(SCD4x_COMMAND_MEASURE_SINGLE_SHOT_RHT_ONLY);

  #if SCD4x_ENABLE_DEBUGLOG
  if (success && (_printDebug == true))
  {
    _debugPort->println(F("SCD4x::measureSingleShot: your data will be ready in 50ms"));
  }
  #endif // if SCD4x_ENABLE_DEBUGLOG

  return (success);
}

scd4x_sensor_type_e SCD4x::getSensorType(void)
{
  return _sensorType;
}

void SCD4x::setSensorType(scd4x_sensor_type_e sensorType)
{
  _sensorType = sensorType;
}

bool SCD4x::getFeatureSetVersion(scd4x_sensor_type_e* sensorType)
{
  if (periodicMeasurementsAreRunning)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true)
    {
      _debugPort->println(F("SCD4x::getFeatureSetVersion: periodic measurements are running. Aborting..."));
    }
    #endif // SCD4x_ENABLE_DEBUGLOG
    return (false);
  }

  uint16_t featureSet;
  
  bool success = readRegister(SCD4x_COMMAND_GET_FEATURE_SET_VERSION, &featureSet, 1);

  #if SCD4x_ENABLE_DEBUGLOG
  if (_printDebug == true)
  {
    _debugPort->print(F("SCD4x::getFeatureSetVersion: Read value: 0x"));
    _debugPort->println(featureSet, HEX);
  }
  #endif // SCD4x_ENABLE_DEBUGLOG

  uint8_t typeOfSensor = ((featureSet & 0x1000) >> 12);

  #if SCD4x_ENABLE_DEBUGLOG
  if (_printDebug == true)
  {
    _debugPort->print(F("SCD4x::getFeatureSetVersion: Type read: 0x"));
    _debugPort->println(typeOfSensor, HEX);
  }
  #endif // SCD4x_ENABLE_DEBUGLOG

  if (typeOfSensor == 0)
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true) 
    {
      _debugPort->println(F("SCD4x::getFeatureSetVersion: Picked SCD40"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    *sensorType = SCD4x_SENSOR_SCD40;
  }
  else if (typeOfSensor == 1) 
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true) 
    {
      _debugPort->println(F("SCD4x::getFeatureSetVersion: Picked SCD41"));
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    *sensorType = SCD4x_SENSOR_SCD41;
  }
  else 
  {
    #if SCD4x_ENABLE_DEBUGLOG
    if (_printDebug == true) {
      if(typeOfSensor == 2)
      {
        _debugPort->println(F("SCD4x::getFeatureSetVersion: SCD42 is not supported by this library."));
      }
      else
      {
        _debugPort->println(F("SCD4x::getFeatureSetVersion: Unknown device type."));
      }
    }
    #endif // if SCD4x_ENABLE_DEBUGLOG
    *sensorType = SCD4x_SENSOR_INVALID;
    success = false;
  }

  return (success);
}

//Sends just a command, no arguments, no CRC
bool SCD4x::sendCommand(uint16_t command) {
    uint8_t cmd[] = {
        (uint8_t)(command >> 8),
        (uint8_t)(command & 0xFF)
    };

    esp_err_t res = i2c_master_write_to_device(I2C_MASTER_NUM, SCD4x_ADDRESS, cmd, sizeof(cmd), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    return (res == ESP_OK);
}

//Sends a command along with arguments and CRC
bool SCD4x::sendCommand(uint16_t command, uint16_t arguments)
{
    uint8_t data[5];  
    data[0] = command >> 8;     // Command MSB
    data[1] = command & 0xFF;   // Command LSB
    data[2] = arguments >> 8;   // Arguments MSB
    data[3] = arguments & 0xFF; // Arguments LSB
    data[4] = computeCRC8(&data[2], 2); // Compute CRC on arguments only

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SCD4x_ADDRESS << 1) | I2C_MASTER_WRITE, true); // Address with write bit
    i2c_master_write(cmd, data, sizeof(data), true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK);
}

//Gets two bytes from SCD4x plus CRC.
//Returns true if endTransmission returns zero _and_ the CRC check is valid
bool SCD4x::readRegister(uint16_t registerAddress, uint16_t *response, uint16_t delayMillis) {
    uint8_t command[] = { 
        (uint8_t)(registerAddress >> 8), 
        (uint8_t)(registerAddress & 0xFF) 
    };

    uint8_t data[3];

    esp_err_t res = i2c_master_write_read_device(I2C_MASTER_NUM, SCD4x_ADDRESS, command, sizeof(command), data, sizeof(data), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    if (res != ESP_OK) return false;

    *response = (data[0] << 8) | data[1];

    return (computeCRC8(data, 2) == data[2]);
}

//Given an array and a number of bytes, this calculate CRC8 for those bytes
//CRC is only calc'd on the data portion (two bytes) of the four bytes being sent
//From: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
//Tested with: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
//x^8+x^5+x^4+1 = 0x31
uint8_t SCD4x::computeCRC8(uint8_t data[], uint8_t len)
{
  uint8_t crc = 0xFF; //Init with 0xFF

  for (uint8_t x = 0; x < len; x++)
  {
    crc ^= data[x]; // XOR-in the next input byte

    for (uint8_t i = 0; i < 8; i++)
    {
      if ((crc & 0x80) != 0)
        crc = (uint8_t)((crc << 1) ^ 0x31);
      else
        crc <<= 1;
    }
  }

  return crc; //No output reflection
}
