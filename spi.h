/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#ifndef SPI_H_
#define SPI_H_

#include <msp430.h>
#include <stdint.h>

// Clock max. on MSP430 is 16 MHz

#define CR_1MHZ 1
#define CR_500KHZ 2
#define CR_250KHZ 4
#define CR_200KHZ 5
#define CR_100KHZ 10
#define CR_50KHZ 20

// Commands

#define RES 0xAB

void spi_init(double clockrate);
void spi_send(uint8_t command);
uint8_t spi_receive();

#endif /* SPI_H_ */
