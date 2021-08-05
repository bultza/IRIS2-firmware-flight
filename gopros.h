/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef GOPROS_H_
#define GOPROS_H_

#include <stdint.h>
#include <msp430.h>
#include "clock.h"

#define CAMERA01 0
#define CAMERA02 1
#define CAMERA03 2
#define CAMERA04 3

#define MUX_OFF         (P2OUT |=  BIT3)
#define CAMERA01_OFF    (P4OUT |=  BIT6)
#define CAMERA01_ON     (P4OUT &= ~BIT6)
#define CAMERA02_OFF    (P4OUT |=  BIT5)
#define CAMERA02_ON     (P4OUT &= ~BIT5)
#define CAMERA03_OFF    (P4OUT |=  BIT4)
#define CAMERA03_ON     (P4OUT &= ~BIT4)
#define CAMERA04_OFF    (P2OUT |=  BIT7)
#define CAMERA04_ON     (P2OUT &= ~BIT7)

#define CAM_WAIT_POWER 5000
#define CAM_WAIT_BUTTON 1000

#define FSM_CAM_DONOTHING           0
#define FSM_CAM_POWERON             1
#define FSM_CAM_POWERON_WAIT        2
#define FSM_CAM_PRESSBTN            3
#define FSM_CAM_PRESSBTN_WAIT       4
#define FSM_CAM_SETTIME             5
#define FSM_CAM_SETTIME_WAIT        6
#define FSM_CAM_PRESSBTNOFF         7
#define FSM_CAM_PRESSBTNOFF_WAIT    8
#define FSM_CAM_POWEROFF_WAIT       9

#define CAM_STATUS_OFF      0
#define CAM_STATUS_ON       1
#define CAM_STATUS_CONF     2
#define CAM_STATUS_PICTURE  3
#define CAM_STATUS_VIDEO    4

struct CameraStatus
{
    uint8_t cameraStatus;
    uint8_t fsmStatus;
    uint64_t lastCommandTime;
    uint64_t sleepTime;
};

struct CameraFSM
{
    void (*cmdPt)(void);   // output function
    uint32_t sleepTime;
    uint8_t nextStates[3];
};


// Functions
int8_t cameraRawPowerOn(uint8_t selectedCamera);
int8_t cameraPowerOn(uint8_t selectedCamera);
int8_t cameraPowerOff(uint8_t selectedCamera);
int8_t cameraFSMcheck();

#endif /* GOPROS_H_ */
