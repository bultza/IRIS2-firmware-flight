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
    //Time values always reset
    confRegister_.lastStateTime = 0;
    confRegister_.lastSubStateTime = 0;

    if(confRegister_.magicWord != MAGICWORD)
    {
        //We need to load the default configuration!!
        confRegister_.magicWord = MAGICWORD;
        confRegister_.swVersion = FWVERSION;

        confRegister_.flightState = 0;
        confRegister_.flightSubState = 0;

        confRegister_.fram_telemetryAddress = FRAM_TLM_ADDRESS;
        confRegister_.fram_eventAddress = FRAM_EVENTS_ADDRESS;
        confRegister_.fram_tlmSavePeriod = FRAM_TLM_SAVEPERIOD;

        confRegister_.nor_deviceSelected = CS_FLASH1;
        confRegister_.nor_telemetryAddress = NOR_TLM_ADDRESS;
        confRegister_.nor_eventAddress = NOR_EVENTS_ADDRESS;
        confRegister_.nor_tlmSavePeriod = NOR_TLM_SAVEPERIOD;

        confRegister_.leds = 1; //1 = on, 0 = off

        //01 = 70%, 02 = off
        confRegister_.gopro_beeps = 02;
        //00 = off, 01 = 2, 02 = 4
        confRegister_.gopro_leds = 0;

        //00 = Gopro Black, 01 = Gopro White
        confRegister_.gopro_model[0] = 00;
        confRegister_.gopro_model[1] = 00;
        confRegister_.gopro_model[2] = 00;
        confRegister_.gopro_model[3] = 00;

        confRegister_.baro_readPeriod = BARO_READPERIOD;
        confRegister_.ina_readPeriod = INA_READPERIOD;
        confRegister_.acc_readPeriod = ACC_READPERIOD;
        confRegister_.temp_readPeriod = TEMP_READPERIOD;
        confRegister_.debugUART = 0;
        confRegister_.sim_enabled = 0;
        confRegister_.sim_sunriseSignal = 0;

        //Remember camera configuration:
        //CAMERA 1: Points the mirror assembly (frontplate)
        //CAMERA 2: Points to the side (horizon + telescope)
        //CAMERA 3: Points to the top (balloon)
        //CAMERA 4: Points to the front (Just landscape and Sun)

        //Launch Configuration
        confRegister_.launch_heightThreshold = 3000;    //3km
        confRegister_.launch_climbThreshold = 2;        //On IRIS1 it was recorded 7.8m/s
        confRegister_.launch_videoDurationLong = 7200;  //2 hours
        confRegister_.launch_videoDurationShort = 3600; //1h
        confRegister_.launch_camerasLong = 0x03;        //Cameras 1 & 2
        confRegister_.launch_camerasShort = 0x0C;       //Cameras 3 & 4
        confRegister_.launch_timeClimbMaximum = 14400;  //4hours (+ 2h of the launch video)

        //Timelapse Configuration
        confRegister_.flight_timelapse_period = TIMELAPSE_PERIOD;
        confRegister_.flight_camerasFirstLeg = 0x0F;    //Cameras 1, 2, 3 & 4
        confRegister_.flight_camerasSecondLeg = 0x0B;   //Cameras 1, 2 & 4
        confRegister_.flight_timeSecondLeg = 86400;     //24 hours

        //Landing Configuration
        confRegister_.landing_heightThreshold = 25000;         //Under this height, the video starts recording
        confRegister_.landing_heightSecurityThreshold = 32000; //Over this height, the vertical speeds are ignored
        confRegister_.landing_speedThreshold = -15;         //On IRIS1 it measured -55m/s at 31.5km height
        confRegister_.landing_videoDurationLong = 3600;     //On IRIS1 decend was 50 minutes
        confRegister_.landing_videoDurationShort = 900;     //This is 15min, on IRIS1 it landed in 10 minutes
        confRegister_.landing_camerasLong = 0x02;       //Camera 2
        confRegister_.landing_camerasShort = 0x09;      //Camera 1 & 4
        confRegister_.landing_camerasHighSpeed = 0x04;  //Camera 3

        confRegister_.landing_heightShortStart = 3000;  //3000km, on IRIS1 it landed 10 minutes later

        //Landed Configuration
        confRegister_.recovery_videoDuration = 120;     //2 minutes of video

        //Reboot counter moves up
        confRegister_.numberReboots++;

#ifdef DEBUG_MODE
        #warning "DEBUG MODE is active. You should never use this mode for the flight version!!"
        //This is just for easy testing in the lab. Disable DEBUG mode for flight!
        confRegister_.gopro_model[3] = 01;
        confRegister_.launch_videoDurationLong = 20;
        confRegister_.launch_videoDurationShort = 10;
        confRegister_.launch_timeClimbMaximum = 60;  //4hours (+ 2h of the launch video)
        confRegister_.flight_timelapse_period = 30;
        confRegister_.flight_timeSecondLeg = 120;
        confRegister_.landing_videoDurationLong = 20;
        confRegister_.landing_videoDurationShort = 10;
        confRegister_.debugUART = 5;    //Report all activity on the COM
        confRegister_.sim_pressure = 101400;

        //01 = 70%, 02 = off
        confRegister_.gopro_beeps = 02;
        //00 = off, 01 = 2, 02 = 4
        confRegister_.gopro_leds = 01;

        confRegister_.leds = 1; //1 = on, 0 = off
#endif

        //Return with error
        return -1;
    }

    //Configuration is already in place so... just move on...
    confRegister_.numberReboots++;

    //Return without error
    return 0;
}
