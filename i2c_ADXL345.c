/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "i2c_ADXL345.h"


int8_t i2c_ADXL345_getAccelerations(int16_t *accelerations)
{
    uint8_t buffer[1];

    buffer[0] = 0x32;   //X-Axis Data 0
    buffer[1] = 0x33;   //X-Axis Data 1
    buffer[2] = 0x34;   //Y-Axis Data 0
    buffer[3] = 0x35;   //Y-Axis Data 1
    buffer[4] = 0x36;   //Z-Axis Data 0
    buffer[5] = 0x37;   //Z-Axis Data 1

    // Read 3-axes acceleration
    int8_t ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, buffer, 6, 0);

    if (ack == 0)   //Request OK
        ack = i2c_requestFrom(I2C_BUS00, ADXL345_ADDRESS, buffer, 6, 0);

    if (ack == 0)   //Retrieval OK
    {
        accelerations[0] = (((int)buffer[1]) << 8) | buffer[0];    //X-Axis
        accelerations[1] = (((int)buffer[3]) << 8) | buffer[2];    //Y-Axis
        accelerations[2] = (((int)buffer[5]) << 8) | buffer[4];    //Z-Axis
    }
    else    //Deterrent values
    {
        accelerations[0] = 32767;
        accelerations[1] = 32767;
        accelerations[2] = 32767;
    }

    return 0;
}




