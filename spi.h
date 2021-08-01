/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#ifndef SPI_H_
#define SPI_H_

#include <msp430.h>
#include <stdint.h>

// Clock max. on MSP430 is 16 MHz

#define CR_8MHZ 0x01
#define CR_4MHZ 0x02
#define CR_2MHZ 0x04
#define CR_1600KHZ 0x05
#define CR_1MHZ 0x08
#define CR_800KHZ 0x0A //10
#define CR_400KHZ 0x14 //20
#define CR_200KHZ 0x28 //40
#define CR_100KHZ 0x50 //80

#define FLASH_CS1_OFF   (P5OUT |=  BIT3)
#define FLASH_CS1_ON    (P5OUT &= ~BIT3)
#define FLASH_CS2_OFF   (P8OUT |=  BIT3)
#define FLASH_CS2_ON    (P8OUT &= ~BIT3)

void spi_init(uint8_t clockrate);
int8_t spi_write_instruction(uint8_t instruction);
int8_t spi_write_read(uint8_t *bufferOut,
                      unsigned int bufferOutLenght,
                      uint8_t *bufferIn,
                      unsigned int bufferInLenght);

#endif /* SPI_H_ */
