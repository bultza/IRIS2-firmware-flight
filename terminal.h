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
#include "configuration.h"
#include "i2c_DS1338Z.h"
#include "i2c_TMP75C.h"
#include "i2c_MS5611.h"
#include "i2c_INA.h"
#include "i2c_ADXL345.h"
#include "datalogger.h"
#include "gopros.h"

#define CMD_MAX_SAVE 10
#define CMD_MAX_LEN 100

#define MEM_CMD_STATUS      0
#define MEM_CMD_READ        1
#define MEM_CMD_DUMP        2
#define MEM_CMD_WRITE       3
#define MEM_CMD_ERASE       4
#define MEM_TYPE_NOR        0
#define MEM_TYPE_FRAM       1
#define MEM_LINE_TLM        0
#define MEM_LINE_EVENT      1
#define MEM_OUTFORMAT_HEX   0
#define MEM_OUTFORMAT_BIN   1

int8_t terminal_start(void);
int8_t terminal_readAndProcessCommands(void);

#endif /* TERMINAL_H_ */
