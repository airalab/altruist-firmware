#include "ags3871.h"

namespace
{
    constexpr TickType_t I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(1000);
    // In the datasheet this bit is named RDY, but its polarity is inverted:
    // RDY=0 means data is ready; RDY=1 means no new data or still warming up.
    constexpr uint8_t RDY_BIT = 0x01;
}

bool AGS3871Driver::begin()
{
    return isConnected();
}

bool AGS3871Driver::isConnected()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr)
    {
        last_i2c_error = ESP_ERR_NO_MEM;
        return false;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    last_i2c_error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_TIMEOUT_TICKS);
    i2c_cmd_link_delete(cmd);

    return last_i2c_error == ESP_OK;
}

AGS3871Error AGS3871Driver::readRegister(uint8_t reg, AGS3871RegisterData &out)
{
    uint8_t buffer[5] = {0, 0, 0, 0, 0};
    const AGS3871Error err = readRawRegister(reg, buffer);

    // Preserve the raw response even on transport errors. This makes debug logs
    // in the wrapper more useful if a partial/stale buffer ever appears.
    for (uint8_t i = 0; i < 4; i++)
    {
        out.data[i] = buffer[i];
    }
    out.crc = buffer[4];

    // The datasheet CRC algorithm returns zero when it is run over
    // Data1..Data4 plus the received CRC byte.
    out.crc_valid = (crc8(buffer, 5) == 0);

    if (err != AGS3871_OK)
    {
        return err;
    }
    return out.crc_valid ? AGS3871_OK : AGS3871_ERROR_CRC;
}

AGS3871Error AGS3871Driver::readConcentration(AGS3871ConcentrationData &out)
{
    AGS3871RegisterData reg;
    const AGS3871Error err = readRegister(REG_CONCENTRATION, reg);
    if (err != AGS3871_OK)
    {
        return err;
    }

    out.status = reg.data[0];
    out.ready = (out.status & RDY_BIT) == 0;
    // Register 0x00 packs concentration as a big-endian 24-bit integer:
    // Data2 is bits 23..16, Data3 is 15..8, Data4 is 7..0.
    out.ppm = (static_cast<uint32_t>(reg.data[1]) << 16) |
              (static_cast<uint32_t>(reg.data[2]) << 8) |
              static_cast<uint32_t>(reg.data[3]);

    return out.ready ? AGS3871_OK : AGS3871_ERROR_NOT_READY;
}

AGS3871Error AGS3871Driver::readVersionRaw(uint32_t &version_raw)
{
    AGS3871RegisterData reg;
    const AGS3871Error err = readRegister(REG_VERSION, reg);
    if (err != AGS3871_OK)
    {
        return err;
    }

    // The datasheet only describes "Data1..Data4" for the version register.
    // Keep all four bytes as a raw value so the wrapper can decide how to print it.
    version_raw = (static_cast<uint32_t>(reg.data[0]) << 24) |
                  (static_cast<uint32_t>(reg.data[1]) << 16) |
                  (static_cast<uint32_t>(reg.data[2]) << 8) |
                  static_cast<uint32_t>(reg.data[3]);
    return AGS3871_OK;
}

AGS3871Error AGS3871Driver::readResistanceOhms(uint32_t &resistance_ohms)
{
    AGS3871RegisterData reg;
    const AGS3871Error err = readRegister(REG_RESISTANCE, reg);
    if (err != AGS3871_OK)
    {
        return err;
    }

    // Datasheet section 4.4 describes Res as a 24-bit value, actual ohms = Res * 10.
    const uint32_t resistance_raw = (static_cast<uint32_t>(reg.data[0]) << 16) |
                                    (static_cast<uint32_t>(reg.data[1]) << 8) |
                                    static_cast<uint32_t>(reg.data[2]);
    resistance_ohms = resistance_raw * 10UL;
    return AGS3871_OK;
}

uint8_t AGS3871Driver::crc8(const uint8_t *data, uint8_t len)
{
    // CRC8 from datasheet section 4.6:
    // init = 0xFF, polynomial = 0x31 (x^8 + x^5 + x^4 + 1).
    uint8_t crc = 0xFF;
    for (uint8_t byte = 0; byte < len; byte++)
    {
        crc ^= data[byte];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x31)
                               : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

AGS3871Error AGS3871Driver::writeRegisterAddress(uint8_t reg)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr)
    {
        last_i2c_error = ESP_ERR_NO_MEM;
        return AGS3871_ERROR_I2C;
    }

    // Select the register to read. The datasheet shows this as the first half
    // of the read command: START, address+W, register, STOP.
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);

    last_i2c_error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_TIMEOUT_TICKS);
    i2c_cmd_link_delete(cmd);

    return last_i2c_error == ESP_OK ? AGS3871_OK : AGS3871_ERROR_I2C;
}

AGS3871Error AGS3871Driver::readFiveBytes(uint8_t *buffer, uint8_t len)
{
    if (buffer == nullptr || len != 5)
    {
        return AGS3871_ERROR_READ;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr)
    {
        last_i2c_error = ESP_ERR_NO_MEM;
        return AGS3871_ERROR_I2C;
    }

    // Read exactly Data1..Data4 + CRC. The last byte must be followed by NACK
    // before STOP, per the I2C read sequence in the datasheet.
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buffer, 4, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, buffer + 4, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    last_i2c_error = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_TIMEOUT_TICKS);
    i2c_cmd_link_delete(cmd);

    return last_i2c_error == ESP_OK ? AGS3871_OK : AGS3871_ERROR_I2C;
}

AGS3871Error AGS3871Driver::readRawRegister(uint8_t reg, uint8_t *buffer)
{
    AGS3871Error err = writeRegisterAddress(reg);
    if (err != AGS3871_OK)
    {
        return err;
    }

    return readFiveBytes(buffer, 5);
}
