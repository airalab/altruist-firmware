#include "CG_RadSens.h"
#include "../../../../utils.h"

// gcc -o test test.cpp radSens1v2.cpp -lwiringPi

CG_RadSens::CG_RadSens()
{
    _sensor_address = RS_DEFAULT_I2C_ADDRESS;
}
CG_RadSens::~CG_RadSens()
{
}

/*Initialization function and sensor connection. Returns false if the sensor is not connected to the I2C bus.*/
bool CG_RadSens::init()
{
    I2cBusLock bus;
    if (!bus.ok()) {
        return false;
    }

    uint8_t reg = 0x00;
    bool res = i2c_write(&reg, 1) == ESP_OK;
    (void)res;
    uint8_t address = getSensorAddress();
    debug_outln_info(F("RadSens address: "), String(address));
    return address != 0;
}

/*Get chip id, default value: 0x7D.*/
uint8_t CG_RadSens::getChipId()
{
    return _chip_id;
}

/*Get firmware version.*/
uint8_t CG_RadSens::getFirmwareVersion()
{
    return _firmware_ver;
}

/*Get radiation intensity (dynamic period T < 123 sec).*/
float CG_RadSens::getRadIntensyDynamic()
{
    updatePulses();
    uint8_t res[3];
    if (i2c_read(RS_RAD_INTENSY_DYNAMIC_RG, res, 3))
    {
        float temp = (((uint32_t)res[0] << 16) | ((uint16_t)res[1] << 8) | res[2]) / 10.0;
        return temp;
    }
    else
    {
        return 0;
    }
}

/*Get radiation intensity (static period T = 500 sec).*/
float CG_RadSens::getRadIntensyStatic()
{
    updatePulses();
    uint8_t res[3];
    if (i2c_read(RS_RAD_INTENSY_STATIC_RG, res, 3))
    {
        return (((uint32_t)res[0] << 16) | ((uint16_t)res[1] << 8) | res[2]) / 10.0;
    }
    else
    {
        return 0;
    }
}

void CG_RadSens::updatePulses()
{
    uint8_t res[2];
    if (i2c_read(RS_PULSE_COUNTER_RG, res, 2))
    {
        _pulse_cnt += (res[0] << 8) | res[1];
    }
}

/*Get the accumulated number of pulses registered by the module
since the last I2C data reading.*/
uint32_t CG_RadSens::getNumberOfPulses()
{
    updatePulses();
    return _pulse_cnt;
}

/*Get sensor address.*/
uint8_t CG_RadSens::getSensorAddress()
{
    uint8_t res;
    if (i2c_read(RS_DEVICE_ADDRESS_RG, &res, 1))
    {
        _sensor_address = res;
        return _sensor_address;
    }
    return 0;
}

/*Get state of high-voltage voltage Converter.*/
bool CG_RadSens::getHVGeneratorState()
{
    uint8_t res;
    if (i2c_read(RS_HV_GENERATOR_RG, &res, 1))
    {
        if (res == 1)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

/*Get the value coefficient used for calculating the radiation intensity.*/
uint16_t CG_RadSens::getSensitivity()
{
    uint8_t res[2];
    if (i2c_read(RS_SENSITIVITY_RG, res, 2))
    {
        return res[1] * 256 + res[0];
    }
    return 0;
}

/*Control register for a high-voltage voltage Converter. By
default, it is in the enabled state. To enable the HV generator,
write 1 to the register, and 0 to disable it. If you try to write other
values, the command is ignored.
 * @param state  true - generator on / false - generator off
 */

bool CG_RadSens::setHVGeneratorState(bool state)
{
    uint8_t data[] = {RS_HV_GENERATOR_RG, state ? 1 : 0};
    return i2c_write(data, sizeof(data)) == ESP_OK;
}

/*Get state of led indication.*/
bool CG_RadSens::getLedState()
{
    uint8_t res;
    if (i2c_read(RS_LED_CONTROL_RG, &res, 1))
    {
        if (res == 1)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

/**
 * Read block of data
 * @param regAddr - address of starting register
 * @param dest -destination array
 * @param num - number of bytes to read
 */
bool CG_RadSens::i2c_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_sensor_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_sensor_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
}


bool CG_RadSens::i2c_write(const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_sensor_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
}