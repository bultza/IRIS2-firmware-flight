/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */
#include "flight_sequence.h"

#define SUBSTATE_OFF            0
#define SUBSTATE_ON_WAITING     1
#define SUBSTATE_PIC_WAITING    2
#define SUBSTATE_OFF_WAITING    3

uint32_t lastTimePicture_ = 0;
uint64_t lastTimeAction_ = 0;
uint8_t flightSubState_ = 0;

/**
 * Private function to see if it is time to start making pictures
 */
void checkStateOff()
{
    //Read Uptime:
    uint32_t uptime = seconds_uptime();
    if(lastTimePicture_ + confRegister_.timelapse_period > uptime)
        return; //Continue waiting

    LED_R_ON;
    //Time to make another picture:
    uint8_t i;

    //Switch on Cameras
    for(i = 0; i < 4; i++)
    {
        gopros_cameraInit(i, CAMERAMODE_PIC);
        cameraPowerOn(i);
    }

    flightSubState_ = SUBSTATE_ON_WAITING;

    //Make sure we did it correctly:
    lastTimePicture_ = uptime;
}

/**
 *
 */
void checkStateOnWaiting()
{
    if(cameraReadyStatus() != 0x0F)
        return;

    //LED_G_ON;
    //Time to make picture!
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        cameraTakePic(i);
    }

    //Write event
    uint8_t payload[5] = {0};
    payload[0] = 1;
    payload[1] = 1;
    payload[2] = 1;
    payload[3] = 1;
    saveEventSimple(EVENT_CAMERA_TIMELAPSE_PIC, payload);

    flightSubState_ = SUBSTATE_PIC_WAITING;
    lastTimeAction_ = millis_uptime();
}

/**
 *
 */
void checkStatePicWaiting()
{
    uint64_t timeNow = millis_uptime();
    if(lastTimeAction_ + 4000 > timeNow)
        return;

    //LED_G_OFF;
    //LED_B_ON;
    //Time to switch off cameras!
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        cameraPowerOff(i);
    }

    flightSubState_ = SUBSTATE_OFF_WAITING;
}

/**
 *
 */
void checkStateOffWaiting()
{
    if(cameraReadyStatus())
        return;
    flightSubState_ = SUBSTATE_OFF;
    //LED_B_OFF;
    LED_R_OFF;
}

/**
 * Checks if there are any activities to be performed during the flight
 */
void checkFlightSequence()
{
    //If we are in debug mode, interrupt any activities:
    if(confRegister_.flightState == FLIGHTSTATE_DEBUG)
        return;

    if(confRegister_.flightState == FLIGHTSTATE_TIMELAPSE)
    {
        //Is it time to make another picture?
        switch(flightSubState_)
        {
        case (SUBSTATE_OFF):
            //We are waiting if it is time to make picture
            checkStateOff();
            break;
        case (SUBSTATE_ON_WAITING):
            checkStateOnWaiting();
            break;
        case (SUBSTATE_PIC_WAITING):
            checkStatePicWaiting();
            break;
        case (SUBSTATE_OFF_WAITING):
            checkStateOffWaiting();
            break;
        default:
            flightSubState_ = SUBSTATE_OFF;
            break;
        }
    }
}
