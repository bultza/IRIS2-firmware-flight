/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */
#include "flight_sequence.h"

#define SUBSTATE_OFF            0
#define SUBSTATE_ON_WAITING     1
#define SUBSTATE_PIC_WAITING    2
#define SUBSTATE_OFF_WAITING    3

#define SUBSTATE_LAUNCH_OFF                  0
#define SUBSTATE_LAUNCH_VIDEO_ON_WAITING     1
#define SUBSTATE_LAUNCH_VIDEO_RECORDING      2
#define SUBSTATE_LAUNCH_VIDEO_OFF_SHORTS     3
#define SUBSTATE_LAUNCH_VIDEO_END            4

uint32_t lastTimePicture_ = 0;

///////////////////////////////////////////////////////////////////////////////
// Functions for the flight timelapse
///////////////////////////////////////////////////////////////////////////////

/**
 * Private function to see if it is time to start making pictures
 */
void checkStateOff()
{
    //Read Uptime:
    uint32_t uptime = seconds_uptime();
    if(lastTimePicture_ + confRegister_.flight_timelapse_period > uptime)
        return; //Continue waiting

    //LED_R_ON;
    //Time to make another picture:
    uint8_t i;

    //Switch on Cameras
    for(i = 0; i < 4; i++)
    {
        gopros_cameraInit(i, CAMERAMODE_PIC);
        cameraPowerOn(i, 0);
    }

    confRegister_.flightSubState = SUBSTATE_ON_WAITING;

    //Make sure we did it correctly:
    lastTimePicture_ = uptime;
}

/**
 *
 */
void checkStateOnWaiting()
{
    uint64_t timeNow = millis_uptime();
    if(cameraReadyStatus() != 0x0F)
    {
        confRegister_.lastSubStateTime = timeNow;
        return;
    }

    //Wait 1s after cameras are ready
    if(confRegister_.lastSubStateTime + 1000 > timeNow)
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

    confRegister_.flightSubState = SUBSTATE_PIC_WAITING;
    confRegister_.lastSubStateTime = millis_uptime();
}

/**
 *
 */
void checkStatePicWaiting()
{
    uint64_t timeNow = millis_uptime();
    if(confRegister_.lastSubStateTime + 1000 > timeNow)
        return;

    //LED_G_OFF;
    //LED_B_ON;
    //Time to switch off cameras!
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        cameraPowerOff(i);
    }

    confRegister_.flightSubState = SUBSTATE_OFF_WAITING;
}

/**
 *
 */
void checkStateOffWaiting()
{
    if(cameraReadyStatus())
        return;
    confRegister_.flightSubState = SUBSTATE_OFF;
    //LED_B_OFF;
    //LED_R_OFF;
}

///////////////////////////////////////////////////////////////////////////////
// Functions for the launch video
///////////////////////////////////////////////////////////////////////////////

/**
 * Start launch video!
 */
void launch_checkStateOff()
{

    //LED_R_ON;
    //Time to make another picture:
    uint8_t i;

    //Switch on Cameras
    for(i = 0; i < 4; i++)
    {
        if(confRegister_.launch_camerasShort[i] == 1
                || confRegister_.launch_camerasLong[i] == 1 )
        {
            gopros_cameraInit(i, CAMERAMODE_VID);
            cameraPowerOn(i, 1);    //In slow mode
        }
    }

    confRegister_.flightSubState = SUBSTATE_LAUNCH_VIDEO_ON_WAITING;
    confRegister_.lastSubStateTime = millis_uptime();

    //Make sure we did it correctly:
    lastTimePicture_ = seconds_uptime();
}

/**
 *
 */
void launch_checkStateOnWaiting()
{
    uint64_t timeNow = millis_uptime();
    /*if(cameraReadyStatus() != 0x0F)
    {
        confRegister_.lastSubStateTime = timeNow;
        return;
    }*/

    //Wait 8s after cameras are ready
    if(confRegister_.lastSubStateTime + 8000 > timeNow)
        return;

    //LED_G_ON;
    //Time to START VIDEO
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        if(confRegister_.launch_camerasShort[i] == 1
            || confRegister_.launch_camerasLong[i] == 1 )
        {
            gopros_cameraStartRecordingVideo(i);
        }
    }

    //Write event
    uint8_t payload[5] = {0};
    payload[0] = confRegister_.launch_camerasShort[0] + 2 * confRegister_.launch_camerasLong[0];
    payload[1] = confRegister_.launch_camerasShort[1] + 2 * confRegister_.launch_camerasLong[1];
    payload[2] = confRegister_.launch_camerasShort[2] + 2 * confRegister_.launch_camerasLong[2];
    payload[3] = confRegister_.launch_camerasShort[3] + 2 * confRegister_.launch_camerasLong[3];
    saveEventSimple(EVENT_CAMERA_VIDEO_START, payload);

    confRegister_.flightSubState = SUBSTATE_LAUNCH_VIDEO_RECORDING;
    confRegister_.lastSubStateTime = millis_uptime();
}

/**
 *
 */
void launch_checkStateVideoWaiting_01()
{
    uint64_t timeNow = millis_uptime();
    if(confRegister_.lastSubStateTime + confRegister_.launch_videoDurationShort > timeNow)
        return;

    //Time to switch off some cameras!
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        //Send command to stop:
        if(confRegister_.launch_camerasShort[i] == 1)
            gopros_cameraStopRecordingVideo(i);
    }

    //Sleep 100ms and switch off
    sleep_ms(100);

    for(i = 0; i < 4; i++)
    {
        if(confRegister_.launch_camerasShort[i] == 1)
            cameraPowerOff(i);
    }

    uint8_t payload[5] = {0};
    payload[0] = confRegister_.launch_camerasShort[0];
    payload[1] = confRegister_.launch_camerasShort[1];
    payload[2] = confRegister_.launch_camerasShort[2];
    payload[3] = confRegister_.launch_camerasShort[3];
    saveEventSimple(EVENT_CAMERA_VIDEO_END, payload);

    confRegister_.flightSubState = SUBSTATE_LAUNCH_VIDEO_OFF_SHORTS;
}

/**
 *
 */
void launch_checkStateVideoWaiting_02()
{
    uint64_t timeNow = millis_uptime();
    if(confRegister_.lastSubStateTime + confRegister_.launch_videoDurationLong > timeNow)
        return;

    //Time to switch off some cameras!
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        //Send command to stop:
        gopros_cameraStopRecordingVideo(i);
    }

    //Sleep 100ms and switch off
    sleep_ms(100);

    for(i = 0; i < 4; i++)
    {
        cameraPowerOff(i);
    }

    uint8_t payload[5] = {0};
    payload[0] = confRegister_.launch_camerasLong[0];
    payload[1] = confRegister_.launch_camerasLong[1];
    payload[2] = confRegister_.launch_camerasLong[2];
    payload[3] = confRegister_.launch_camerasLong[3];
    saveEventSimple(EVENT_CAMERA_VIDEO_END, payload);

    confRegister_.flightSubState = SUBSTATE_LAUNCH_VIDEO_END;
}

/**
 *
 */
uint8_t launch_checkStateOffWaiting()
{
    if(cameraReadyStatus())
        return 0;

    //Return that it is time to move on
    return 1;
}

/**
 * Checks if there are any activities to be performed during the flight
 */
void checkFlightSequence()
{
    //If we are in debug mode, interrupt any activities:
    if(confRegister_.flightState == FLIGHTSTATE_DEBUG)
        return;

    uint64_t uptime = millis_uptime();

    if(confRegister_.flightState == FLIGHTSTATE_WAITFORLAUNCH)
    {
        //Avoid measuring heights at switch on
        if(uptime < 5000)
            return;

        //Are we very high or climbing very fast?
        if(   (getAltitude() > (confRegister_.launch_heightThreshold * 100))
           || (getVerticalSpeed() > (confRegister_.launch_climbThreshold * 100))
           || getSunriseSignalActivated() )
        {
            //Lets move to the next state!!
            confRegister_.flightState = FLIGHTSTATE_LAUNCH;
            confRegister_.lastStateTime = uptime;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime;
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_LAUNCH;
            payload[1] = getAltitude() > (confRegister_.launch_heightThreshold * 100);
            payload[2] = getVerticalSpeed() > (confRegister_.launch_climbThreshold * 100);
            payload[3] = getSunriseSignalActivated();
            saveEventSimple(EVENT_STATE_CHANGED, payload);
        }
        else
        {
            //We continue waiting
        }

    }

    if(confRegister_.flightState == FLIGHTSTATE_LAUNCH)
    {
        //Make videos during launch
        switch(confRegister_.flightSubState)
        {
        case (SUBSTATE_LAUNCH_OFF):
            launch_checkStateOff();
            break;
        case (SUBSTATE_LAUNCH_VIDEO_ON_WAITING):
            launch_checkStateOnWaiting();
            break;
        case (SUBSTATE_LAUNCH_VIDEO_RECORDING):
            launch_checkStateVideoWaiting_01();
            break;
        case (SUBSTATE_LAUNCH_VIDEO_OFF_SHORTS):
            launch_checkStateVideoWaiting_02();
            break;
        case (SUBSTATE_LAUNCH_VIDEO_END):
            if(launch_checkStateOffWaiting())
            {
                //Cameras are off, we move to the next step:
                confRegister_.flightState = FLIGHTSTATE_TIMELAPSE;
                confRegister_.lastStateTime = uptime;
                //Reset the substate
                confRegister_.flightSubState = 0;
                confRegister_.lastSubStateTime = uptime;
                uint8_t payload[5] = {0};
                payload[0] = FLIGHTSTATE_TIMELAPSE;
                saveEventSimple(EVENT_STATE_CHANGED, payload);
            }
            break;
        default:
            confRegister_.flightSubState = SUBSTATE_LAUNCH_OFF;
            break;
        }
    }

    if(confRegister_.flightState == FLIGHTSTATE_TIMELAPSE)
    {
        //Is it time to make another picture?
        switch(confRegister_.flightSubState)
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
            confRegister_.flightSubState = SUBSTATE_OFF;
            break;
        }

        int32_t verticalSpeed = getVerticalSpeed();
        int32_t altitude = getAltitude();
        if(altitude > confRegister_.landing_heightSecurityThreshold)
            verticalSpeed = 0;  //avoid listening to vertical speeds when altitude is too high


        if(   (getAltitude() < (confRegister_.landing_heightThreshold * 100))
           || (verticalSpeed < (confRegister_.landing_speedThreshold * 100))
           || getSunriseSignalActivated() )
        {
            //Lets move to the next state!!
            confRegister_.flightState = FLIGHTSTATE_LANDING;
            confRegister_.lastStateTime = uptime;

            if(confRegister_.flightSubState != SUBSTATE_OFF)
            {
                //Joder!! Giouder!! we got the signal when doing a picture! Just cut the power please...
                //TODO Maybe change it to something without state machines, just with some delays...
                uint8_t i;
                for(i = 0; i < 4; i++)
                {
                    cameraPowerOffUnsafe(i);
                }

            }

            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime;

            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_LANDING;
            payload[1] = getAltitude() < (confRegister_.landing_heightThreshold * 100);
            payload[2] = verticalSpeed < (confRegister_.landing_speedThreshold * 100);
            payload[3] = getSunriseSignalActivated();
            saveEventSimple(EVENT_STATE_CHANGED, payload);
        }
        else
        {
            //We continue waiting
        }

    }

    if(confRegister_.flightState == FLIGHTSTATE_LANDING)
    {
        //Is it time to make another picture?
        switch(confRegister_.flightSubState)
        {
        //TODO
        default:
            confRegister_.flightSubState = SUBSTATE_OFF;
            break;
        }
    }

    if(confRegister_.flightState == FLIGHTSTATE_TIMELAPSE_LAND)
    {
        //Is it time to make another picture?
        switch(confRegister_.flightSubState)
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
            confRegister_.flightSubState = SUBSTATE_OFF;
            break;
        }

        //TODO Detect the shaking so we start making video!!!
    }

    if(confRegister_.flightState == FLIGHTSTATE_RECOVERY)
    {
        //Is it time to make another picture?
        switch(confRegister_.flightSubState)
        {
        //TODO
        default:
            confRegister_.flightSubState = SUBSTATE_OFF;
            break;
        }
    }


}
