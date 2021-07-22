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

int8_t i2c_ADXL345_init(void);
int8_t i2c_ADXL345_getAccelerations(int16_t *accelerations);

#endif /* I2C_ADXL345_H_ */
