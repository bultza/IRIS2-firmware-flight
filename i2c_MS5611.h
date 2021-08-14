/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef I2C_MS5611_H_
#define I2C_MS5611_H_


#include <msp430.h>
#include <stdint.h>
#include <math.h>
#include "i2c.h"
#include "clock.h"

#define MS5611_ADDRESS 0x77
#define MS5611_RESET 0x1E
#define MS5611_REG_C1 0xA2
#define MS5611_REG_C2 0xA4
#define MS5611_REG_C3 0xA6
#define MS5611_REG_C4 0xA8
#define MS5611_REG_C5 0xAA
#define MS5611_REG_C6 0xAC
#define MS5611_CONVERT_D1 0x48
#define MS5611_CONVERT_D2 0x58
#define MS5611_ADC_READ 0x00

#define Pmin 10     // 10 mbar
#define Pmax 1200   // 1200 mbar
#define Tmin -40    // -40 deg C
#define Tmax 85     // 85 deg C
#define Tref 20     // 20 deg C

int8_t i2c_MS5611_init(void);
int8_t i2c_MS5611_getPressure(int32_t * pressure);
int32_t calculateAltitude(int32_t pressure);

#endif /* I2C_MS5611_H_ */
