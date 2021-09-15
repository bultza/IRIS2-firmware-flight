/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#include "configuration.h"

#pragma PERSISTENT (confRegister_)
struct ConfigurationRegister confRegister_ = {0};

/**
 *
 */
int8_t configuration_init(void)
{
    if(confRegister_.magicWord != MAGICWORD)
    {
        //We need to load the default configuration!!
        confRegister_.magicWord = MAGICWORD;
        confRegister_.swVersion = FWVERSION;
        confRegister_.simulatorEnabled = 0;
        confRegister_.flightState = 0;
        confRegister_.flightSubState = 0;

        confRegister_.fram_telemetryAddress = FRAM_TLM_ADDRESS;
        confRegister_.fram_eventAddress = FRAM_EVENTS_ADDRESS;
        confRegister_.fram_tlmSavePeriod = FRAM_TLM_SAVEPERIOD;

        confRegister_.nor_deviceSelected = CS_FLASH1;
        confRegister_.nor_telemetryAddress = NOR_TLM_ADDRESS;
        confRegister_.nor_eventAddress = NOR_EVENTS_ADDRESS;
        confRegister_.nor_tlmSavePeriod = NOR_TLM_SAVEPERIOD;

        confRegister_.baro_readPeriod = BARO_READPERIOD;
        confRegister_.ina_readPeriod = INA_READPERIOD;
        confRegister_.acc_readPeriod = ACC_READPERIOD;
        confRegister_.temp_readPeriod = TEMP_READPERIOD;
        confRegister_.debugUART = 0;

        //TODO

        //Reboot counter moves up
        confRegister_.numberReboots++;

        //Return with error
        return -1;
    }

    //Configuration is already in place so... just move on...
    confRegister_.numberReboots++;

    //Return without error
    return 0;
}
