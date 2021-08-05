#include "configuration.h"


#pragma PERSISTENT (confRegister_)
struct ConfigurationRegister confRegister_ = {0};

/**
 *
 */
void configuration_init(void)
{
    if(confRegister_.magicWord != MAGICWORD)
    {
        //We need to load the default configuration!!
        confRegister_.magicWord = MAGICWORD;
        confRegister_.simulatorEnabled = 0;
        confRegister_.swVersion = FWVERSION;

        confRegister_.fram_telemetryAddress = FRAM_TLM_ADDRESS;
        confRegister_.fram_logAddress = FRAM_EVENTS_ADDRESS;

        //TODO
    }

    //Configuration is already in place so... just move on...
    confRegister_.numberReboots++;
}
