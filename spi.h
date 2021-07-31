/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#ifndef SPI_H_
#define SPI_H_

#include <msp430.h>
#include <stdint.h>

// Clock max. on MSP430 is 16 MHz

#define CR_8MHZ 1
#define CR_4MHZ 2
#define CR_2MHZ 4
#define CR_1600KHZ 5
#define CR_1MHZ 8
#define CR_800KHZ 10
#define CR_400KHZ 20
#define CR_200KHZ 40
#define CR_100KHZ 80

#define FLASH_CS1_OFF   (P5OUT |=  BIT3)
#define FLASH_CS1_ON    (P5OUT &= ~BIT3)
#define FLASH_CS2_OFF   (P3OUT |=  BIT6)
#define FLASH_CS2_ON    (P3OUT &= ~BIT6)

void spi_init(uint8_t clockrate);
int8_t spi_write_instruction(uint8_t instruction);
int8_t spi_write_read(uint8_t *bufferOut,
                      unsigned int bufferOutLenght,
                      uint8_t *bufferIn,
                      unsigned int bufferInLenght);

#endif /* SPI_H_ */
