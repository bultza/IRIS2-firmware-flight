/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef I2C_TMP75C_H_
#define I2C_TMP75C_H_


#include <msp430.h>
#include <stdint.h>
#include "i2c.h"

#define TMP75_ADDRESS01 0x48
#define TMP75_ADDRESS02 0x4C
#define TMP75_ADDRESS03 0x4D

//#define TMP75_ADDRESS02 0x4a
//#define TMP75_ADDRESS03 0x4b
//#define TMP75_ADDRESS04 0x4c
//#define TMP75_ADDRESS05 0x4d

#define TMP75_REG_TEMP  0x00
#define TMP75_REG_CONF  0x01
#define TMP75_REG_TLOW  0x02
#define TMP75_REG_THIGH 0x03

int8_t i2c_TMP75_init(void);
int8_t i2c_TMP75_getTemperatures(int16_t *temperatures);

#endif /* I2C_TMP75C_H_ */
