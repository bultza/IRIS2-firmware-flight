/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#ifndef SPI_NOR_H_
#define SPI_NOR_H_

#include <msp430.h>
#include <stdint.h>
#include "spi.h"
#include "configuration.h"
#include "clock.h"

// Device is S70FS70FL01GS - 1 Gbit NOR Flash memory.
// Device includes two smaller S25FL512 memories - 512 Mbit each.
// The memory to be read/written from/to must be first selected with CS.

// Select one device or the other
#define CS_FLASH1   0
#define CS_FLASH2   1

// Commands for single S25FL512 memories
#define NOR_RDID        0x9F        // Read Identification
#define NOR_READ        0x03        // Read from 3-bit address
#define NOR_FOURREAD    0x13        // Read from 4-bit address
#define NOR_WREN        0x06        // Write Enable
#define NOR_WRDI        0x04        // Write Disable
#define NOR_PP          0x02        // Write to 3-byte address
#define NOR_FOURPP      0x12        // Write to 4-byte address
#define NOR_SE          0xD8        // Sector Erase a 3-byte address (only one sector)
#define NOR_FOURSE      0xDC        // Sector Erase a 4-byte address (only one sector)
#define NOR_BE          0x60        // Bulk Erase (entire flash memory array)
#define NOR_RDSR1       0x05        // Read Status Register 1

// Constants
#define NOR_BYTES_PAGE      512         //512 Bytes per page
#define NOR_BYTES_SECTOR    256000      //256 kB per sector
#define NOR_NUM_SECTORS     256
#define NOR_NUM_PAGES       128000

#define NOR_OPERATION_IDLE  0
#define NOR_OPERATION_BULK  1
#define NOR_OPERATION_DUMP  2

// Structures
struct RDIDInfo
{
    int8_t manufacturerID;
    int8_t memoryInterfaceType;
    int8_t density;
    int8_t cfiLength;
    int8_t sectorArchitecture;
    int8_t familyID;
    int8_t asciiModelChars1;
    int8_t asciiModelChars2;
};

struct NOR_Status
{
    int32_t timeStart;
    uint8_t operationOngoing;
};

// Functions
int8_t spi_NOR_init(uint8_t deviceSelect);
int8_t spi_NOR_checkWriteInProgress(uint8_t deviceSelect);
int8_t spi_NOR_getRDID(struct RDIDInfo *idInformation, uint8_t deviceSelect);
int8_t spi_NOR_readFromAddress(uint32_t readAddress, uint8_t * buffer, uint16_t numOfBytes, uint8_t deviceSelect);
int8_t spi_NOR_writeToAddress(uint32_t writeAddress, uint8_t * buffer, uint16_t numOfBytes, uint8_t deviceSelect);
int8_t spi_NOR_sectorErase(uint32_t sectorAddress, uint8_t deviceSelect);
int8_t spi_NOR_bulkErase(uint8_t deviceSelect);
void checkMemory();

#endif /* SPI_NOR_H_ */
