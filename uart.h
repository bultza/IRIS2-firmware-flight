/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#ifndef UART_H_
#define UART_H_

#include <msp430.h>
#include <stdint.h>
#include "configuration.h"

//******************************************************************************
//* PUBLIC TYPE DEFINITIONS :                                                  *
//******************************************************************************
//Baudrates
#define BR_9600 0
#define BR_38400 1
#define BR_57600 2
#define BR_115200 3

//Base Address (from datasheet)
#define DEBUG_BASE 0x05C0  //Eusci_A0
#define CAM1_BASE  0x05E0  //Eusci_A1
#define CAM2_BASE  0x0600  //Eusci_A2
#define CAM3_BASE  0x0620  //Eusci_A3

#define UART_DEBUG 0
#define UART_CAM1  1
#define UART_CAM2  2
#define UART_CAM3  3
#define UART_CAM4  4

#define UARTBUFFERLENGHT 1024


//******************************************************************************
//* PUBLIC FUNCTION DECLARATIONS :                                             *
//******************************************************************************
int8_t uart_init(uint8_t uart_name, uint8_t baudrate);
void uart_close(uint8_t uart_name);

int8_t uart_write(uint8_t uart_name,
                  uint8_t *buffer,
                  uint16_t length);

int8_t uart_print(uint8_t uart_name,
                  char *buffer);

uint8_t uart_read(uint8_t uart_name);

int16_t uart_available(uint8_t uart_name);
void uart_clear_buffer(uint8_t uart_name);


#endif /* UART_H_ */
