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

#define MAGICWORD           0xBABE
#define FWVERSION           3
//#define FRAM_TLM_SAVEPERIOD 600     //seconds period to save on FRAM
#define FRAM_TLM_SAVEPERIOD 60     //Only for Debug
#define NOR_TLM_SAVEPERIOD  10      //seconds period to save on NOR Flash
#define BARO_READPERIOD     1000    //Milliseconds period to read barometer
#define TEMP_READPERIOD     1000    //Milliseconds period to read temperatures
#define INA_READPERIOD      100     //Milliseconds period to read INA Voltage and currents
#define ACC_READPERIOD      100     //Milliseconds period to read Accelerometer


struct ConfigurationRegister
{
    uint16_t magicWord;
    uint16_t numberReboots;
    uint16_t swVersion;
    //Put here all the configuration parameters
    //TODO
    uint16_t nor_tlmSavePeriod;
    uint16_t fram_tlmSavePeriod;
    uint16_t baro_readPeriod;
    uint16_t temp_readPeriod;
    uint16_t ina_readPeriod;
    uint16_t acc_readPeriod;

    //Put here all the current execution status
    uint32_t nor_eventAddress;
    uint32_t nor_telemetryAddress;
    uint32_t fram_eventAddress;
    uint32_t fram_telemetryAddress;
    //TODO
    uint8_t simulatorEnabled;
    uint8_t flightState;
    uint8_t flightSubState;
    uint16_t hardwareRebootReason;
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
