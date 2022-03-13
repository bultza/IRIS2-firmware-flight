/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#include "spi_NOR.h"

struct NOR_Status nor_status_;

// PRIVATE FUNCTIONS

/**
 *
 */
int8_t NOR_writeEnableDisable(uint8_t enabled, uint8_t deviceSelect)
{
    //LED_B_ON;
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    if (enabled)
        spi_write_instruction(NOR_WREN);
    else
        spi_write_instruction(NOR_WRDI);

    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    //LED_B_OFF;

    return 0;
}

// PUBLIC FUNCTIONS

/**
 * Returns whether the NOR memory is busy or not.
 */
int8_t spi_NOR_checkWriteInProgress(uint8_t deviceSelect)
{
    LED_B_ON;
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    uint8_t bufferOut[1] = {NOR_RDSR1};
    uint8_t bufferIn[1];
    spi_write_read(bufferOut, 1, bufferIn, 1);

    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    LED_B_OFF;

    if (bufferIn[0] & 0x01)
        return 1;
    else
        return 0;
}


/**
 * Requests the Identification of the NOR memory.
 */
int8_t spi_NOR_getRDID(struct RDIDInfo *idInformation, uint8_t deviceSelect)
{
    LED_B_ON;
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    uint8_t bufferOut[1] = {NOR_RDID};
    uint8_t bufferIn[8];

    //__delay_cycles(500);
    spi_write_read(bufferOut, 1, bufferIn, 8);
    //__delay_cycles(500);

    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    idInformation->manufacturerID = bufferIn[0];
    idInformation->memoryInterfaceType = bufferIn[1];
    idInformation->density = bufferIn[2];
    idInformation->cfiLength = bufferIn[3];
    idInformation->sectorArchitecture = bufferIn[4];
    idInformation->familyID = bufferIn[5];
    idInformation->asciiModelChars1 = bufferIn[6];
    idInformation->asciiModelChars2 = bufferIn[7];

    LED_B_OFF;

    return 0;
}

/**
 * Read bytes from an address.
 */
int8_t spi_NOR_readFromAddress(uint32_t readAddress, uint8_t * buffer, uint16_t numOfBytes, uint8_t deviceSelect)
{
    // Check that no write operation is in progress, else return error
    if (spi_NOR_checkWriteInProgress(deviceSelect))
        return -1;

    LED_B_ON;
    // Chip Select ON
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    // Build bufferOut of bytes to send
    uint8_t bufferOut[5];
    bufferOut[0] = NOR_FOURREAD;
    bufferOut[1] = (uint8_t) (((readAddress & 0xFF000000) >> 24) & 0xFF);
    bufferOut[2] = (uint8_t) (((readAddress & 0x00FF0000) >> 16) & 0xFF);
    bufferOut[3] = (uint8_t) (((readAddress & 0x0000FF00) >> 8) & 0xFF);
    bufferOut[4] = (uint8_t) ((readAddress & 0x000000FF) & 0xFF);

    // Send bufferOut, collect bytres read
    spi_write_read(bufferOut, 5, buffer, numOfBytes);

    // Chip Select OFF
    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    LED_B_OFF;

    return 0;
}

/**
 * Writes from 1 byte to 512 consecutive bytes (1 page)
 * starting from a provided address. If the end of the page
 * is reached, the subsequent write address is set to the beginning
 * of the page.
 */
//TODO: Implement error return with P_ERR from SR1.
int8_t spi_NOR_writeToAddress(uint32_t writeAddress, uint8_t * buffer, uint16_t numOfBytes, uint8_t deviceSelect)
{
    // Check that no write operation is in progress, else return error
    if (spi_NOR_checkWriteInProgress(deviceSelect))
        return -1;

    LED_B_ON;

    // Enable Write Operations
    NOR_writeEnableDisable(1, deviceSelect);

    // Chip Select ON
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    // Build bufferOut of bytes to send
    uint8_t bufferTotalLength = 5 + numOfBytes; // 5 bytes required for fixed part
    uint8_t bufferOut[NOR_BYTES_PAGE];
    bufferOut[0] = NOR_FOURPP;
    bufferOut[1] = (uint8_t) (((writeAddress & 0xFF000000) >> 24) & 0xFF);
    bufferOut[2] = (uint8_t) (((writeAddress & 0x00FF0000) >> 16) & 0xFF);
    bufferOut[3] = (uint8_t) (((writeAddress & 0x0000FF00) >> 8) & 0xFF);
    bufferOut[4] = (uint8_t) ((writeAddress & 0x000000FF) & 0xFF);

    uint8_t i;
    for (i = 5; i < bufferTotalLength; i++)
    {
        bufferOut[i] = buffer[i-5];
    }

    // Send bufferOut, do not expect any answer in return (buffer variable used as placeholder)
    spi_write_read(bufferOut, bufferTotalLength, buffer, 0);

    // Chip Select OFF
    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    // Disable Write Operations
    NOR_writeEnableDisable(0, deviceSelect);

    LED_B_OFF;

    return 0;
}

/**
 * Erases (sets bits to 1) a whole sector (512 bytes) given its address.
 */
//TODO: Implement error return with E_ERR from SR1.
int8_t spi_NOR_sectorErase(uint32_t sectorAddress, uint8_t deviceSelect)
{
    // Check that no write operation is in progress, else return error
    if (spi_NOR_checkWriteInProgress(deviceSelect))
        return -1;

    LED_B_ON;

    // Enable Write Operations
    NOR_writeEnableDisable(1, deviceSelect);

    // Chip Select ON
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    // Build bufferOut of bytes to send
    uint8_t bufferOut[5];
    bufferOut[0] = NOR_FOURSE;
    bufferOut[1] = (uint8_t) (((sectorAddress & 0xFF000000) >> 24) & 0xFF);
    bufferOut[2] = (uint8_t) (((sectorAddress & 0x00FF0000) >> 16) & 0xFF);
    bufferOut[3] = (uint8_t) (((sectorAddress & 0x0000FF00) >> 8) & 0xFF);
    bufferOut[4] = (uint8_t) ((sectorAddress & 0x000000FF) & 0xFF);

    // Send bufferOut, do not expect any answer in return (buffer variable used as placeholder)
    spi_write_read(bufferOut, 5, bufferOut, 0);

    // Chip Select OFF
    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    // Wait until Sector Erase has finished
    while(spi_NOR_checkWriteInProgress(deviceSelect));

    // Disable Write Operations
    NOR_writeEnableDisable(0, deviceSelect);

    LED_B_OFF;

    return 0;
}

/**
 * Erases (sets bits to 1) the whole flash memory, takes about 120 seconds.
 */
int8_t spi_NOR_bulkErase(uint8_t deviceSelect)
{
    // Check that no write operation is in progress, else return error
    if (spi_NOR_checkWriteInProgress(deviceSelect))
        return -1;

    LED_B_ON;

    // Enable Write Operations
    NOR_writeEnableDisable(1, deviceSelect);

    // Chip Select ON
    if (deviceSelect == CS_FLASH1)
        FLASH_CS1_ON;
    else if (deviceSelect == CS_FLASH2)
        FLASH_CS2_ON;

    // Build bufferOut of bytes to send
    spi_write_instruction(NOR_BE);

    // Chip Select OFF
    FLASH_CS1_OFF;
    FLASH_CS2_OFF;

    nor_status_.operationOngoing = NOR_OPERATION_BULK;
    nor_status_.timeStart = seconds_uptime();

    //// Wait until Sector Erase has finished
    ////while(spi_NOR_checkWriteInProgress(deviceSelect));

    //// Disable Write Operations
    //NOR_writeEnableDisable(0, deviceSelect);

    LED_B_OFF;

    return 0;
}

/**
 * It checks if there is any operation ongoing and pushes it
 */
void checkMemory()
{
    if(nor_status_.operationOngoing == NOR_OPERATION_IDLE)
        return;

    if(nor_status_.operationOngoing == NOR_OPERATION_BULK)
    {
        if(spi_NOR_checkWriteInProgress(confRegister_.nor_deviceSelected))
            return; //still busy

        //Finished!
        NOR_writeEnableDisable(0, confRegister_.nor_deviceSelected);
        int32_t eraseEnd = seconds_uptime();
        char strToPrint[100];
        sprintf(strToPrint, "NOR memory bulk erase completed in %ld seconds.\r\n# ", eraseEnd - nor_status_.timeStart);
        uart_print(UART_DEBUG, strToPrint);
        nor_status_.operationOngoing = NOR_OPERATION_IDLE;
    }
}
