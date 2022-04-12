/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef I2C_H_
#define I2C_H_


#include <msp430.h>
#include <stdint.h>
//#include "configuration.h"
#include "clock.h"
#include "libhal.h"

//******************************************************************************
//* PUBLIC TYPE DEFINITIONS :                                                  *
//******************************************************************************

#define I2C_BUS00 0
#define I2C_BUS01 1

//#define I2CTIMEOUTCYCLES    10000UL   //30000 for 8MHz at 400kHz
#define I2CTIMEOUTCYCLES    5000UL   //30000 for 8MHz at 400kHz


//******************************************************************************
//* PUBLIC FUNCTION DECLARATIONS :                                             *
//******************************************************************************
void i2c_master_init();
int8_t i2c_begin_transmission(uint8_t busSelect, uint8_t address,
                              uint8_t asReceiver);
int8_t i2c_write_firstbyte(uint8_t busSelect, uint8_t *buffer);
int8_t i2c_write(uint8_t busSelect, uint8_t address, uint8_t *buffer,
                     uint16_t length, uint8_t repeatedStart);
int8_t i2c_requestFrom(uint8_t busSelect, uint8_t address, uint8_t *buffer,
                           uint16_t length, uint8_t repeatedStart);


#endif /* I2C_H_ */
