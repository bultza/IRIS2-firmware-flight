/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <stdint.h>
#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uart.h"
#include "clock.h"
#include "i2c_DS1338Z.h"
#include "i2c_TMP75C.h"
#include "i2c_MS5611.h"
#include "i2c_INA.h"
#include "gopros.h"

#define CMD_MAX_LEN 100

int8_t terminal_start(void);
int8_t terminal_readAndProcessCommands(void);

#endif /* TERMINAL_H_ */
