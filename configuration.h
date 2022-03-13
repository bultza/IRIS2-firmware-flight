/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdint.h>
#include <msp430.h>
#include "spi_NOR.h"
#include "datalogger.h"

#define LED_B_OFF       (P3OUT |= BIT6)
#define LED_B_ON        (P3OUT &= ~BIT6)
#define LED_B_TOGGLE    (P3OUT ^= BIT6)

#define LED_G_OFF       (P3OUT |= BIT5)
#define LED_G_ON        (P3OUT &= ~BIT5)
#define LED_G_TOGGLE    (P3OUT ^= BIT5)

#define LED_R_OFF       (P3OUT |= BIT4)
#define LED_R_ON        (P3OUT &= ~BIT4)
#define LED_R_TOGGLE    (P3OUT ^= BIT4)

#define MAGICWORD           0xBABE
#define FWVERSION           8
//#define FRAM_TLM_SAVEPERIOD 599     //seconds period to save on FRAM
#define FRAM_TLM_SAVEPERIOD 599     //Only for Debug TODO
#define NOR_TLM_SAVEPERIOD  10      //seconds period to save on NOR Flash
#define BARO_READPERIOD     1000    //Milliseconds period to read barometer
#define TEMP_READPERIOD     1000    //Milliseconds period to read temperatures
#define INA_READPERIOD      100     //Milliseconds period to read INA Voltage and currents
#define ACC_READPERIOD      100     //Milliseconds period to read Accelerometer
#define TIMELAPSE_PERIOD    120      //Seconds
//#define TIMELAPSE_PERIOD    30      //Seconds

#define FLIGHTSTATE_DEBUG           0
#define FLIGHTSTATE_WAITFORLAUNCH   1
#define FLIGHTSTATE_LAUNCH          2
#define FLIGHTSTATE_TIMELAPSE       3
#define FLIGHTSTATE_LANDING         4
#define FLIGHTSTATE_TIMELAPSE_LAND  5
#define FLIGHTSTATE_RECOVERY        6

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

    //Put here all the configuration of the gopros
    uint8_t gopro_beeps;        //01 = 70%, 02 = off
    uint8_t gopro_leds;         //00 = off, 01 = 2, 02 = 4
    uint8_t gopro_model[4];     //00 = Gopro Black, 01 = Gopro White (TouchScreen)


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
    uint8_t  launch_camerasLong[4];
    uint8_t  launch_camerasShort[4];
    uint16_t launch_timeClimbMaximum;   //s

    //Timelapse Configuration
    uint16_t flight_timelapse_period;   //s
    uint8_t  flight_camerasFirstLeg[4];
    uint8_t  flight_camerasSecondLeg[4];
    uint32_t flight_timeSecondLeg;      //s

    //Landing Configuration
    int32_t landing_heightThreshold;         //m Under this height, the video starts recording
    int32_t landing_heightSecurityThreshold; //m Over this height, the vertical speeds are ignored
    int32_t  landing_speedThreshold;          //m/s
    uint16_t landing_videoDurationLong;       //s
    uint16_t landing_videoDurationShort;      //s
    uint8_t  landing_camerasLong[4];
    uint8_t  landing_camerasShort[4];
    int32_t landing_heightShortStart;        //m ->3000m

    //Landed Configuration
    uint16_t recovery_videoDuration;

    //For debug
    uint8_t debugUART;
    uint8_t sim_enabled;
    int32_t sim_pressure;
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
