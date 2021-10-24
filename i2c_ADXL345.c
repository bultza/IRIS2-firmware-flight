/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "i2c_ADXL345.h"

/**
 * Configures the accelerometer, so it can start to make measurements returns
 * the ack value of the accelerometer.
 */
int8_t i2c_ADXL345_init(void)
{
    // Take accelerometer out of Sleep mode and activate BIT to start measuring
    uint8_t buffer[2] = {ADXL345_POWER_CTL, 0x08};
    int8_t ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 2, 0);

    if (ack == 0)
    {
        // Set format of measurements:
        //full resolution (13-bit), full resolution
        buffer[0] = ADXL345_DATA_FORMAT;
        buffer[1] = 0x0B;
        ack |= i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 2, 0);

        //Set output at 25Hz, bandwidth 12.5Hz 0x08
        buffer[0] = ADXL345_BW_RATE;
        buffer[1] = 0x08;
        ack |= i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 2, 0);

        //set values for what is considered freefall (0-255)
        //   adxl.setFreeFallThreshold(7); //(5 - 9) recommended - 62.5mg per increment
        buffer[0] = ADXL345_THRESH_FF;
        buffer[1] = 0x07;
        ack |= i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 2, 0);

        //   adxl.setFreeFallDuration(45); //(20 - 70) recommended - 5ms per increment
        buffer[0] = ADXL345_TIME_FF;
        buffer[1] = 45;
        ack |= i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 2, 0);

        //Set all interrupts to pin INT1 on the chip ( = 0x00)
        buffer[0] = ADXL345_INT_MAP;
        buffer[1] = 0x00;
        ack |= i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 2, 0);

        //Activate only the Free Fall interrupt = 0x04
        buffer[0] = ADXL345_INT_ENABLE;
        buffer[1] = 0x04;
        ack |= i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 2, 0);
    }

    return ack;
}

/**
 * Returns the x, y, z readings of the acc in g
 */
int8_t i2c_ADXL345_getAccelerations(struct ACCData *data)
{
    uint8_t adxlRegister = ADXL345_DATAX0;
    uint8_t adxlData[6];

    int8_t ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, &adxlRegister, 1, 0);

    if (ack)
        return ack; //There was an error, return the error

    // Retrieve XAXIS0, XAXIS1, YAXIS0, YAXIS1, ZAXIS0, ZAXIS1
    ack |= i2c_requestFrom(I2C_BUS00, ADXL345_ADDRESS, adxlData, 6, 0);

    data->x = (((int16_t) ((adxlData[1] << 8) | adxlData[0])) * 4);
    data->y = (((int16_t) ((adxlData[3] << 8) | adxlData[2])) * 4);
    data->z = (((int16_t) ((adxlData[5] << 8) | adxlData[4])) * 4);
    return ack;
}

/**
 * Returns the free fall interrupt information
 */
int8_t i2c_ADXL345_getIntStatus(uint8_t *interruptRegister,
                                uint8_t *interruptDetected,
                                uint8_t *gpioStatus)
{
    uint8_t adxlRegister = ADXL345_INT_SOURCE;
    int8_t ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, &adxlRegister, 1, 0);

    if (ack)
        return ack; //There was an error, return the error

    // Retrieve register
    ack |= i2c_requestFrom(I2C_BUS00, ADXL345_ADDRESS, interruptRegister, 1, 0);

    if(P3IN & BIT7)
        *gpioStatus = 1;
    else
        *gpioStatus = 0;

    //TODO add the interrupt reader information

    return ack;
}
