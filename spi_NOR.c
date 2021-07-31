/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */


#include "spi_NOR.h"

// Private functions
int8_t NOR_selectDevice(uint8_t deviceSelect);

int8_t NOR_selectDevice(uint8_t deviceSelect)
{
    // By default, unselect the two NOR memories
    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    // Now select the requested one
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    return 0;
}

/**
 * Requests the Identification of the NOR memory.
 */
int8_t spi_NOR_getRDID(struct RDIDInfo *idInformation, uint8_t deviceSelect)
{
    NOR_selectDevice(deviceSelect);

    uint8_t bufferOut[1] = {NOR_RDID};
    uint8_t bufferIn[8];

    spi_write_read(bufferOut, 1, bufferIn, 8);

    idInformation->manufacturerID = bufferIn[0];
    idInformation->memoryInterfaceType = bufferIn[1];
    idInformation->density = bufferIn[2];
    idInformation->cfiLength = bufferIn[3];
    idInformation->sectorArchitecture = bufferIn[4];
    idInformation->familyID = bufferIn[5];
    idInformation->asciiModelChars1 = bufferIn[6];
    idInformation->asciiModelChars2 = bufferIn[7];

    return 0;
}
