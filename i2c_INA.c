/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "i2c_INA.h"

/**
 * Configure the INA with the PCB and resistor details
 */
int8_t i2c_INA_init(void)
{
    uint8_t buffer[3];

    //Read the Manufacturer ID Register (0xFE) must be 21577
    buffer[0] = INA_ID;
    int8_t ack = i2c_write(I2C_BUS00, INA_ADDRESS, buffer, 1, 0);
    if (ack != -1)
        i2c_requestFrom(I2C_BUS00, INA_ADDRESS, buffer, 2, 0);
    else
        return ack;

    /**
     *to select the number, go to INA226 datasheet section 7.6.1
     *Register 0x00 Configuration Register
     *Selected options: 0100(0 reset and 100 default value)/
     *(011) 64 value averages/(101) 2.116ms bus voltage conversion time/
     *(101) 2.116ms shunt voltage conversion/ (111)Shunt and bus, continuous
     */
    buffer[0] = INA_CONF_REG;
    buffer[1] = 0x47;
    buffer[2] = 0x6F;
    i2c_write(I2C_BUS00, INA_ADDRESS, buffer, 3, 0);

    /**
     * Register 0x05 Calibration
     * CAL = 0.00512/(Current_LSB * Rshunt) = 2560 converted on hex = 0xA00
     * Used the example value of 1mA resolution
     *
     * Current_LSB = Max expected current / 2^15, Rshunt = 0.002 ohm
     */
    buffer[0] = INA_CAL_REG; //here goes the register address (Calibration register 0x05)
    buffer[1] = 0x0A;
    buffer[2] = 0x00;
    i2c_write(I2C_BUS00, INA_ADDRESS, buffer, 3, 0);

    buffer[0] = INA_MASK_REG; //here goes the mask address (Calibration register 0x06)
    buffer[1] = 0x08;
    buffer[2] = 0x00;
    return i2c_write(I2C_BUS00, INA_ADDRESS, buffer, 3, 0);
}

/**
 * Read voltage and current
 */
int8_t i2c_INA_read(struct INAData *data)
{
    uint8_t buffer[2];

    data->error = 0;

    // Order to read the Current Register (0x04)
    // it gives the value in amperes * 10000
    buffer[0] = INA_CURRENT;
    int8_t ack = i2c_write(I2C_BUS00, INA_ADDRESS, buffer, 1, 0);
    if (ack != -1)
    {
        i2c_requestFrom(I2C_BUS00, INA_ADDRESS, buffer, 2, 0);
        //Make conversion
        data->current = (0xFF00 & ((uint16_t) buffer[0] << 8))
                | ((uint16_t) (0x00FF & buffer[1]));
    }
    else
    {
        data->current= 0;
        data->error = -1;
    }

    // Order to read the Bus Voltage Register (0x02)
    // it gives the value in volts * 100
    buffer[0] = INA_VBUS;
    ack = i2c_write(I2C_BUS00, INA_ADDRESS, buffer, 1, 0);
    if (ack != -1)
    {
        i2c_requestFrom(I2C_BUS00, INA_ADDRESS, buffer, 2, 0);
        //Make conversion to number of bits
        data->voltage = (uint16_t) ((uint32_t) ((0xFF00 & ((uint16_t) buffer[0] << 8))
                | ((uint16_t) (0x00FF & buffer[1]))) * 125 / 1000);

    }
    else
    {
        data->voltage = 0;
        data->error += -1;
    }

    return data->error;

}
