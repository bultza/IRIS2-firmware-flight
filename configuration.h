/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

//#define DEBUG_MODE
#define MAGICWORD           0xBABE
#define FWVERSION           14

#include <stdint.h>
#include <msp430.h>
#include "spi_NOR.h"
#include "datalogger.h"

#define FRAM_TLM_SAVEPERIOD 600     //seconds period to save on FRAM
#define NOR_TLM_SAVEPERIOD  10      //seconds period to save on NOR Flash
#define BARO_READPERIOD     1000    //Milliseconds period to read barometer
#define TEMP_READPERIOD     1000    //Milliseconds period to read temperatures
#define INA_READPERIOD      100     //Milliseconds period to read INA Voltage and currents
#define ACC_READPERIOD      100     //Milliseconds period to read Accelerometer
#define TIMELAPSE_PERIOD    120      //Seconds

//Define all the flight status available for the flight plan
#define FLIGHTSTATE_DEBUG           0
#define FLIGHTSTATE_WAITFORLAUNCH   1
#define FLIGHTSTATE_LAUNCH          2
#define FLIGHTSTATE_TIMELAPSE       3
#define FLIGHTSTATE_LANDING         4
#define FLIGHTSTATE_TIMELAPSE_LAND  5
#define FLIGHTSTATE_RECOVERY        6

//Define all the events that we want to store on the memories
#define EVENT_BOOT                          69
#define EVENT_CONFIGURATION_CHANGED         1
#define EVENT_NOR_CLEAN                     2
#define EVENT_CAMERA_ON                     10
#define EVENT_CAMERA_PICTURE                11
#define EVENT_CAMERA_VIDEO_START            12
#define EVENT_CAMERA_VIDEO_END              13
#define EVENT_CAMERA_OFF                    14
#define EVENT_CAMERA_TIMELAPSE_PIC          15
#define EVENT_CAMERA_VIDEOMODE              16
#define EVENT_CAMERA_PICMODE                17
#define EVENT_CAMERA_VIDEO_INTERRUPT        18
#define EVENT_CAMERA_SDCARD_FORMAT          19
#define EVENT_STATE_CHANGED                 20
#define EVENT_LOW_ALTITUDE_DETECTED         30
#define EVENT_MOVEMENT_DETECTED             40
#define EVENT_I2C_ERROR_RESET               99
#define EVENT_SUNRISE_GPIO_CHANGE           100
#define EVENT_SUNRISE_SIGNAL_DETECTED       101
#define EVENT_BATTERY_CUTOUT                200

//WDT configuration:
//Select SMCLK as the source of wdt, clear WDT and set to 2^31 (268,43s)
//#define DAE_WDTKICK     (WDTSSEL_0 + WDTCNTCL)
//Select SMCLK as the source of wdt, clear WDT and set to 2^27 (16.77s)
#define DAE_WDTKICK     (WDTSSEL_0 + WDTCNTCL + WDTIS0)
//Select SMCLK as the source of wdt, clear WDT and set to 2^23 (1.04s)
//#define DAE_WDTKICK     (WDTSSEL_0 + WDTCNTCL + WDTIS1)
//Select SMCLK as the source of wdt, clear WDT and set to 2^19 (0.065s)
//#define DAE_WDTKICK     (WDTSSEL_0 + WDTCNTCL + WDTIS1 + WDTIS0)


struct ConfigurationRegister
{
    uint16_t magicWord;
    uint16_t numberReboots;
    uint16_t swVersion;
    //Put here all the configuration parameters
    uint16_t nor_tlmSavePeriod;
    uint16_t fram_tlmSavePeriod;
    uint16_t baro_readPeriod;
    uint16_t temp_readPeriod;
    uint16_t ina_readPeriod;
    uint16_t acc_readPeriod;
    uint8_t leds;

    //Put here all the configuration of the gopros
    uint8_t gopro_beeps;        //01 = 70%, 02 = off
    uint8_t gopro_leds;         //00 = off, 01 = 2, 02 = 4
    uint8_t gopro_model[4];     //00 = Gopro Black, 01 = Gopro White (TouchScreen)
    uint16_t gopro_pictureSleep; //2500 ms by default. less is too risky (depends on the sd-card)


    //Put here all the current execution status
    uint8_t nor_deviceSelected;
    uint32_t nor_eventAddress;
    uint32_t nor_telemetryAddress;
    uint32_t fram_eventAddress;
    uint32_t fram_telemetryAddress;
    uint16_t hardwareRebootReason;

    //States
    uint8_t flightState;
    uint8_t flightSubState;
    uint64_t lastStateTime;
    uint64_t lastSubStateTime;

    //Launch Configuration
    int32_t launch_heightThreshold;    //m
    int32_t launch_climbThreshold;     //m/s
    uint16_t launch_videoDurationLong;  //s
    uint16_t launch_videoDurationShort; //s
    uint8_t  launch_camerasLong;
    uint8_t  launch_camerasShort;
    uint16_t launch_timeClimbMaximum;   //s

    //Timelapse Configuration
    uint16_t flight_timelapse_period;   //s
    uint8_t  flight_camerasFirstLeg;
    uint8_t  flight_camerasSecondLeg;
    uint32_t flight_timeSecondLeg;      //s

    //Landing Configuration
    int32_t landing_heightThreshold;         //m Under this height, the video starts recording
    int32_t landing_heightSecurityThreshold; //m Over this height, the vertical speeds are ignored
    int32_t  landing_speedThreshold;          //m/s
    uint16_t landing_videoDurationLong;       //s
    uint16_t landing_videoDurationShort;      //s
    uint8_t  landing_camerasLong;
    uint8_t  landing_camerasShort;
    uint8_t  landing_camerasHighSpeed;
    int32_t landing_heightShortStart;        //m ->3000m

    //Landed Configuration
    uint16_t recovery_videoDuration;

    //For debug
    uint8_t debugUART;
    uint8_t sim_enabled;
    int32_t sim_pressure;
    uint8_t sim_sunriseSignal;    //1 if high, 2 if low simulated (remember it has a pull up)

    //For RTC drift
    int16_t rtcDrift;               //s, every X seconds you need to update the RTC by 1s
    uint8_t rtcDriftFlag;           //0 = disabled, 1 = enabled clock is low so +1, 2 = enabled clock is fast so -1

    uint32_t rtcLastTimeUpdate;
};

//******************************************************************************
//* PUBLIC VARIABLE DECLARATIONS :                                             *
//******************************************************************************
extern struct ConfigurationRegister confRegister_;

//******************************************************************************
//* PUBLIC FUNCTION DECLARATIONS :                                             *
//******************************************************************************
int8_t configuration_init(void);



#endif /* CONFIGURATION_H_ */
