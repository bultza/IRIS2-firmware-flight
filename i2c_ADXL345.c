/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "i2c_ADXL345.h"

int8_t i2c_ADXL345_init(void)
{
    // Take accelerometer out of Sleep mode, start measuring
    uint8_t powerControl[2] = {ADXL345_REG_POWERCTL, ADXL345_INITIALIZE};
    // Set format of measurements: full resolution (13-bit), full resolution
    uint8_t dataFormat[2] = {ADXL345_REG_DATAFMT, ADXL345_FORMAT};

    int8_t ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, powerControl, 2, 0);

    if (ack == 0)
        ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, dataFormat, 2, 0);

    return ack;
}

int8_t i2c_ADXL345_getAccelerations(struct Accelerations *accData)
{
    uint8_t adxlRegister = ADXL345_REG_XAXIS0;
    uint8_t adxlData[6];

    int8_t ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, &adxlRegister, 1, 0);

    if (ack == 0) // Retrieve XAXIS0, XAXIS1, YAXIS0, YAXIS1, ZAXIS0, ZAXIS1
        ack = i2c_requestFrom(I2C_BUS00, ADXL345_ADDRESS, adxlData, 6, 0);

    if (ack == 0) // Process obtained values
    {
        accData->XAxis = (float) ((int16_t) ((adxlData[1] << 8) | adxlData[0])) * (32.0/(2^13));
        accData->YAxis = (float) ((int16_t) ((adxlData[3] << 8) | adxlData[2])) * (32.0/(2^13));
        accData->ZAxis = (float) ((int16_t) ((adxlData[5] << 8) | adxlData[4])) * (32.0/(2^13));
    }
    else // Error: return deterrent values
    {
        accData->XAxis = 32767;
        accData->YAxis = 32767;
        accData->ZAxis = 32767;
    }

    return ack;
}

int8_t i2c_ADXL345_setAxesOffsets(int16_t xAxisOffset, int16_t yAxisOffset, int16_t zAxisOffset)
{
    uint8_t const adxlRegister = ADXL345_REG_XAXISOFF;
    uint8_t adxlData[4] = {adxlRegister, xAxisOffset, yAxisOffset, zAxisOffset};

    int8_t ack = i2c_write(I2C_BUS00, ADXL345_ADDRESS, adxlData, 4, 0);

    return ack;
}



