/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdint.h>
#include <msp430.h>
#include "datalogger.h"

#define MAGICWORD   0xBABE
#define FWVERSION   2
#define FRAM_TLM_SAVEPERIOD 600     //seconds period to save on FRAM
#define NOR_TLM_SAVEPERIOD 10       //seconds period to save on NOR Flash


struct ConfigurationRegister
{
    uint16_t magicWord;
    uint16_t numberReboots;
    uint16_t swVersion;
    //Put here all the configuration parameters
    //TODO
    uint16_t nor_tlmSavePeriod;
    uint16_t fram_tlmSavePeriod;

    //Put here all the current execution status
    uint32_t nor_logAddress;
    uint32_t nor_telemetryAddress;
    uint32_t fram_logAddress;
    uint32_t fram_telemetryAddress;
    //TODO
    uint8_t simulatorEnabled;
};

//******************************************************************************
//* PUBLIC VARIABLE DECLARATIONS :                                             *
//******************************************************************************
extern struct ConfigurationRegister confRegister_;

//******************************************************************************
//* PUBLIC FUNCTION DECLARATIONS :                                             *
//******************************************************************************
void configuration_init(void);



#endif /* CONFIGURATION_H_ */
