/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef I2C_ADXL345_H_
#define I2C_ADXL345_H_


#include <msp430.h>
#include <stdint.h>
#include "i2c.h"

#define ADXL345_ADDRESS 0x1D
#define ADXL345_REG_POWERCTL 0x2D
#define ADXL345_REG_DATAFMT 0x31
#define ADXL345_REG_XAXIS0 0x32
#define ADXL345_REG_XAXIS1 0x33
#define ADXL345_REG_YAXIS0 0x34
#define ADXL345_REG_YAXIS1 0x35
#define ADXL345_REG_ZAXIS0 0x36
#define ADXL345_REG_ZAXIS1 0x37
#define ADXL345_REG_XAXISOFF 0x1E // Can be used when instrument is calibrated
#define ADXL345_REG_YAXISOFF 0x1F // Can be used when instrument is calibrated
#define ADXL345_REG_ZAXISOFF 0x20 // Can be used when instrument is calibrated

#define ADXL345_INITIALIZE 0b00001000 // Initialize measurements
#define ADXL345_FORMAT 0b00001011 // Set full resolution, 16g measurement


struct Accelerations
{
    float XAxis;
    float YAxis;
    float ZAxis;
};

int8_t i2c_ADXL345_init(void);
int8_t i2c_ADXL345_getAccelerations(struct Accelerations *accData);
int8_t i2c_ADXL345_setAxesOffsets(int16_t xAxisOffset, int16_t yAxisOffset, int16_t zAxisOffset);

#endif /* I2C_ADXL345_H_ */
