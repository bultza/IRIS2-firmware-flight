/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */
#include "flight_sequence.h"

#define SUBSTATE_LANDING_WAITING               0
#define SUBSTATE_LANDING_VIDEO_STARTED         1
#define SUBSTATE_LANDING_VIDEO_SHORTSSTARTED   2

uint32_t lastTimePicture_ = 0;


/**
 * Checks if there are any activities to be performed during the flight
 */
void checkFlightSequence()
{
    //If we are in debug mode, interrupt any activities:
    if(confRegister_.flightState == FLIGHTSTATE_DEBUG)
        return;

    uint64_t uptime_ms = millis_uptime();

    if(confRegister_.flightState == FLIGHTSTATE_WAITFORLAUNCH)
    {
        //Avoid measuring heights at switch on
        if(uptime_ms < 10000)
            return;

        //Are we very high or climbing very fast?
        if(   (getAltitude() > (confRegister_.launch_heightThreshold * 100))
           || (getVerticalSpeed() > (confRegister_.launch_climbThreshold * 100))
           || getSunriseSignalActivated() )
        {
            //Start making video!!
            uint8_t i;
            for(i = 0; i < 4; i++)
            {
                if((confRegister_.launch_camerasLong >> i) & 1)
                {
                    cameraMakeVideo(i,
                                    CAMERAMODE_VID,
                                    confRegister_.launch_videoDurationLong);
                }
                if((confRegister_.launch_camerasShort >> i) & 1)
                {
                    cameraMakeVideo(i,
                                    CAMERAMODE_VID,
                                    confRegister_.launch_videoDurationShort);
                }
            }

            //Lets move to the next state!!
            confRegister_.flightState = FLIGHTSTATE_LAUNCH;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_LAUNCH;
            payload[1] = getAltitude() > (confRegister_.launch_heightThreshold * 100);
            payload[2] = getVerticalSpeed() > (confRegister_.launch_climbThreshold * 100);
            payload[3] = getSunriseSignalActivated();
            saveEventSimple(EVENT_STATE_CHANGED, payload);

            //Write event
            payload[0] = confRegister_.launch_camerasShort;
            payload[1] = confRegister_.launch_camerasLong;
            payload[2] = 0;
            payload[3] = 0;
            payload[4] = 0;
            saveEventSimple(EVENT_CAMERA_VIDEO_START, payload);
        }
        else
        {
            //We continue waiting
        }

    }

    if(confRegister_.flightState == FLIGHTSTATE_LAUNCH)
    {
        //Just wait for the duration of the longest video + 30s
        if(confRegister_.lastStateTime
                + (uint64_t)confRegister_.launch_videoDurationLong * 1000UL
                + 30000
                < uptime_ms)
        {
            //Cameras are off, we move to the next step:
            confRegister_.flightState = FLIGHTSTATE_TIMELAPSE;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_TIMELAPSE;
            saveEventSimple(EVENT_STATE_CHANGED, payload);
        }

    }

    if(confRegister_.flightState == FLIGHTSTATE_TIMELAPSE)
    {
        uint32_t uptime_s = (uptime_ms / 1000UL);
        //Is it time to make another picture?
        if(lastTimePicture_ + (uint32_t)confRegister_.flight_timelapse_period < uptime_s)
        {
            uint8_t i;
            uint8_t camerasIndex;

            //Decide on which cameras
            if(uptime_s > (confRegister_.lastStateTime/1000UL + confRegister_.flight_timeSecondLeg))
                camerasIndex =  confRegister_.flight_camerasSecondLeg;
            else
                camerasIndex =  confRegister_.flight_camerasFirstLeg;

            for(i = 0; i < 4; i++)
            {
                //Switch on depending on when it is now:
                if((camerasIndex >> i) & 1)
                {
                    cameraTakePicture(i);
                }
            }
            //Write event
            uint8_t payload[5] = {0};
            payload[0] = camerasIndex;
            saveEventSimple(EVENT_CAMERA_TIMELAPSE_PIC, payload);
            lastTimePicture_ = uptime_s;
        }

        int32_t verticalSpeed = getVerticalSpeed();
        int32_t altitude = getAltitude();
        if(altitude > confRegister_.landing_heightSecurityThreshold * 100)
            verticalSpeed = 0;  //avoid listening to vertical speeds when altitude is too high


        if(    (getAltitude() < (confRegister_.landing_heightThreshold * 100))
            || (verticalSpeed < (confRegister_.landing_speedThreshold * 100))
            || getSunriseSignalActivated()
          )
        {
            //Move only if safe descending time reached:
            if(confRegister_.lastStateTime
                    + (uint64_t)confRegister_.launch_timeClimbMaximum * 1000UL
                    < uptime_ms)
            {
                //Lets move to the next state!!
                confRegister_.flightState = FLIGHTSTATE_LANDING;
                confRegister_.lastStateTime = uptime_ms;

                //Reset the substate
                confRegister_.flightSubState = SUBSTATE_LANDING_WAITING;
                confRegister_.lastSubStateTime = uptime_ms;

                uint8_t payload[5] = {0};
                payload[0] = FLIGHTSTATE_LANDING;
                payload[1] = getAltitude() < (confRegister_.landing_heightThreshold * 100);
                payload[2] = verticalSpeed < (confRegister_.landing_speedThreshold * 100);
                payload[3] = getSunriseSignalActivated();
                saveEventSimple(EVENT_STATE_CHANGED, payload);
            }
        }
        else
        {
            //We continue waiting
        }

    }

    if(confRegister_.flightState == FLIGHTSTATE_LANDING)
    {
        if(confRegister_.lastStateTime + 15000 > uptime_ms)
        {
            //During the first 15s after changing, do not start this sequence
            //in order to avoid starting to take video while doing a picture
            //on the timelapse
            return;
        }

        if(confRegister_.flightSubState == SUBSTATE_LANDING_WAITING)
        {
            //Start making video:
            uint8_t i;
            for(i = 0; i < 4; i++)
            {
                if((confRegister_.landing_camerasHighSpeed >> i) & 1)
                {
                    cameraMakeVideo(i,
                                    CAMERAMODE_VID_HIGHSPEED,
                                    confRegister_.landing_videoDurationShort);
                }
                else if((confRegister_.landing_camerasLong >> i) & 1)
                {
                    cameraMakeVideo(i,
                                    CAMERAMODE_VID,
                                    confRegister_.landing_videoDurationLong);
                }
                else if((confRegister_.landing_camerasShort >> i) & 1)
                {
                    cameraMakeVideo(i,
                                    CAMERAMODE_VID,
                                    confRegister_.landing_videoDurationShort);
                }
            }

            //Write event
            uint8_t payload[5] = {0};
            payload[0] = confRegister_.landing_camerasShort;
            payload[1] = confRegister_.landing_camerasLong;
            payload[2] = confRegister_.landing_camerasHighSpeed;

            saveEventSimple(EVENT_CAMERA_VIDEO_START, payload);
            confRegister_.flightSubState = SUBSTATE_LANDING_VIDEO_STARTED;
            confRegister_.lastSubStateTime = uptime_ms;
        }
        else if(confRegister_.flightSubState == SUBSTATE_LANDING_VIDEO_STARTED)
        {
            int32_t altitude = getAltitude();
            //Now waiting to reach heights below 3000m to start recording
            //video on the shorts
            if(altitude < confRegister_.landing_heightShortStart * 100L)
            {
                uint8_t i;
                for(i = 0; i < 4; i++)
                {
                    if((confRegister_.landing_camerasShort >> i) & 1)
                    {
                        cameraMakeVideo(i,
                                        CAMERAMODE_VID,
                                        confRegister_.landing_camerasShort);
                    }else if((confRegister_.landing_camerasHighSpeed >> i) & 1)
                    {
                        cameraMakeVideo(i,
                                        CAMERAMODE_VID,
                                        confRegister_.landing_videoDurationShort);
                    }
                }
                uint8_t payload[5] = {0};
                memcpy(payload, (const uint8_t*)&altitude, 4);
                saveEventSimple(EVENT_LOW_ALTITUDE_DETECTED, payload);
                confRegister_.flightSubState = SUBSTATE_LANDING_VIDEO_SHORTSSTARTED;
                confRegister_.lastSubStateTime = uptime_ms;
            }
        }

        //Lets wait for the duration of the long video plus 30 minutes to go
        //to next step
        if(confRegister_.lastStateTime
                + (uint64_t)confRegister_.landing_videoDurationShort * 1000UL
                + 1800000UL
                < uptime_ms)
        {
            //Cameras are off, we move to the next step:
            confRegister_.flightState = FLIGHTSTATE_TIMELAPSE_LAND;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_TIMELAPSE_LAND;
            saveEventSimple(EVENT_STATE_CHANGED, payload);
            //Activate interrupt for detecting the recovery team:
            i2c_ADXL345_activateActivityDetection();
        }
    }

    if(confRegister_.flightState == FLIGHTSTATE_TIMELAPSE_LAND)
    {
        uint32_t uptime_s = (uptime_ms / 1000UL);
        //Is it time to make another picture?
        if(lastTimePicture_ + (uint32_t)confRegister_.flight_timelapse_period < uptime_s)
        {
            uint8_t i;

            for(i = 0; i < 4; i++)
            {
                //Switch on depending on when it is now:
                cameraTakePicture(i);
            }

            //Write event
            uint8_t payload[5] = {0};
            payload[0] = 0x0F;   //All four cameras taking video
            saveEventSimple(EVENT_CAMERA_TIMELAPSE_PIC, payload);
            lastTimePicture_ = uptime_s;
        }

        if(i2c_ADXL345_getMovementDetected() > 3)
        {
            uint8_t i;
            for(i = 0; i < 4; i++)
            {
                cameraMakeVideo(i,
                                CAMERAMODE_VID,
                                confRegister_.recovery_videoDuration);
            }

            //Write event
            uint8_t payload[5] = {0};
            saveEventSimple(EVENT_CAMERA_VIDEO_START, payload);


            confRegister_.flightSubState = SUBSTATE_LANDING_VIDEO_STARTED;
            confRegister_.lastSubStateTime = uptime_ms;

            //we move to the next step:
            confRegister_.flightState = FLIGHTSTATE_RECOVERY;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;

            payload[0] = FLIGHTSTATE_RECOVERY;
            saveEventSimple(EVENT_STATE_CHANGED, payload);
        }
    }

    if(confRegister_.flightState == FLIGHTSTATE_RECOVERY)
    {
        //Remain here for ever
        uint32_t uptime_s = (uptime_ms / 1000UL);
        //Is it time to make another picture?
        if(lastTimePicture_ + (uint32_t)confRegister_.flight_timelapse_period < uptime_s)
        {
            uint8_t i;

            for(i = 0; i < 4; i++)
            {
                //Switch on depending on when it is now:
                cameraTakePicture(i);
            }

            //Write event
            uint8_t payload[5] = {0};
            payload[0] = 0x0F;   //All four cameras taking video
            saveEventSimple(EVENT_CAMERA_TIMELAPSE_PIC, payload);
            lastTimePicture_ = uptime_s;
        }
    }
}
