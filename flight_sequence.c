/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */
#include "flight_sequence.h"

#define SUBSTATE_LANDING_WAITING               0
#define SUBSTATE_LANDING_VIDEO_STARTED         1
#define SUBSTATE_LANDING_VIDEO_SHORTSSTARTED   2

uint32_t lastTimePicture_ = 0;

int32_t launchHeight_ = 0;
uint8_t  launchDetectedFromSunrise_ = 0;

int32_t landingHeight_ = 0;
uint8_t  landingDetectedFromSunrise_ = 0;

uint64_t triggersLastTime_ = 0;
uint16_t verticalSpeedTrigger_ = 0;
uint16_t heightTrigger_ = 0;

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

        uint8_t sunriseGpioSignal = sunrise_GPIO_Read_Signal();
        uint8_t heightReached = 0;
        uint8_t verticalSpeedReached = 0;

        //We count 10 times to see if threshold was achieved
        if(triggersLastTime_ + 1000 < uptime_ms)
        {
            triggersLastTime_ = uptime_ms;
            int32_t altitude = getAltitude();
            if(altitude > (confRegister_.launch_heightThreshold * 100))
                heightTrigger_++;
            else
                heightTrigger_ = 0;

            if(getVerticalSpeed() > (confRegister_.launch_climbThreshold * 100))
                verticalSpeedTrigger_++;
            else
                verticalSpeedTrigger_ = 0;

            if(getBaroIsOnError())
            {
                //Ignore all measurements from the baro:
                heightTrigger_ = 0;
                verticalSpeedTrigger_ = 0;
            }

            if(heightTrigger_ > ALTITUDE_HISTORY + 1)
                heightReached = 1;

            if(verticalSpeedTrigger_ > ALTITUDE_HISTORY + 1)
                verticalSpeedReached = 1;
        }

        //Are we very high or climbing very fast?
        if( heightReached
           || verticalSpeedReached
           || sunriseGpioSignal)
        {
            //Start making video!!
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_LAUNCH;
            payload[1] = heightReached;
            payload[2] = verticalSpeedReached;
            payload[3] = sunriseGpioSignal;
            saveEventSimple(EVENT_STATE_CHANGED, payload);

            //Lets move to the next state!!
            confRegister_.flightState = FLIGHTSTATE_LAUNCH;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;
            heightTrigger_ = 0;
            verticalSpeedTrigger_ = 0;

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

            if(sunriseGpioSignal)
            {
                //TRigger was Sunrise, register it just in case we have to go
                //back and do it only if barometer is actually working:
                if(getBaroIsOnError() == 0)
                {
                    launchDetectedFromSunrise_ = 1;
                    launchHeight_ = getAltitude(); //In cm!!
                }
            }
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
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_TIMELAPSE;
            saveEventSimple(EVENT_STATE_CHANGED, payload);

            //Cameras are off, we move to the next step:
            confRegister_.flightState = FLIGHTSTATE_TIMELAPSE;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;
        }
        else
        {
            if(launchDetectedFromSunrise_
                    && confRegister_.lastStateTime  + 600000L < uptime_ms)
            {
                int32_t currentAltitude = getAltitude();
                if(currentAltitude - launchHeight_ < 10000L
                        && getBaroIsOnError() == 0)
                {
                    //On Sunrise 1 it measured > 4km in 10min, here
                    //we did less than 100m in 10min, so interrupt

                    uint8_t payload[5] = {0};
                    payload[0] = FLIGHTSTATE_WAITFORLAUNCH;
                    payload[1] = 0xFF;
                    saveEventSimple(EVENT_STATE_CHANGED, payload);

                    //Cameras are off, we move to the next step:
                    confRegister_.flightState = FLIGHTSTATE_WAITFORLAUNCH;
                    confRegister_.lastStateTime = uptime_ms;
                    //Reset the substate
                    confRegister_.flightSubState = 0;
                    confRegister_.lastSubStateTime = uptime_ms;

                    uint8_t i;
                    for(i = 0; i < 4; i++)
                    {
                        cameraInterruptVideo(i);
                    }
                }
            }
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

            lastTimePicture_ = uptime_s;
        }

        uint8_t sunriseGpioSignal = sunrise_GPIO_Read_Signal();
        uint8_t heightReached = 0;
        uint8_t verticalSpeedReached = 0;

        //We count 10 times to see if threshold was achieved and once per second
        if(triggersLastTime_ + 1000 < uptime_ms)
        {
            triggersLastTime_ = uptime_ms;
            int32_t altitude = getAltitude();
            int32_t verticalSpeed = getVerticalSpeed();
            if(altitude > confRegister_.landing_heightSecurityThreshold * 100)
                verticalSpeed = 0;  //avoid listening to vertical speeds when altitude is too high

            if(altitude < (confRegister_.landing_heightThreshold * 100))
                heightTrigger_++;
            else
                heightTrigger_ = 0;

            if(getVerticalSpeed() < (confRegister_.landing_speedThreshold * 100))
                verticalSpeedTrigger_++;
            else
                verticalSpeedTrigger_ = 0;

            if(getBaroIsOnError())
            {
                //Ignore all measurements from the baro:
                heightTrigger_ = 0;
                verticalSpeedTrigger_ = 0;
            }

            if(heightTrigger_ > ALTITUDE_HISTORY + 1)
                heightReached = 1;

            if(verticalSpeedTrigger_ > ALTITUDE_HISTORY + 1)
                verticalSpeedReached = 1;
        }

        if( heightReached
            || verticalSpeedReached
            || sunriseGpioSignal)
        {
            //Move only if safe descending time reached:
            if(confRegister_.lastStateTime
                    + (uint64_t)confRegister_.launch_timeClimbMaximum * 1000UL
                    < uptime_ms)
            {
                uint8_t payload[5] = {0};
                payload[0] = FLIGHTSTATE_LANDING;
                payload[1] = heightReached;
                payload[2] = verticalSpeedReached;
                payload[3] = sunriseGpioSignal;
                saveEventSimple(EVENT_STATE_CHANGED, payload);

                //Lets move to the next state!!
                confRegister_.flightState = FLIGHTSTATE_LANDING;
                confRegister_.lastStateTime = uptime_ms;

                //Reset the substate
                confRegister_.flightSubState = SUBSTATE_LANDING_WAITING;
                confRegister_.lastSubStateTime = uptime_ms;
                heightTrigger_ = 0;
                verticalSpeedTrigger_ = 0;

                if(sunriseGpioSignal)
                {
                    //TRigger was Sunrise, register it just in case we have to go
                    //back and do it only if barometer is actually working:
                    if(getBaroIsOnError() == 0)
                    {
                        landingDetectedFromSunrise_ = 1;
                        landingHeight_ = getAltitude(); //In cm!!
                    }
                }
            }
        }
        else
        {
            //We continue waiting
        }

    }

    if(confRegister_.flightState == FLIGHTSTATE_LANDING)
    {
        if(confRegister_.lastStateTime + 15000L > uptime_ms)
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
                uint8_t payload[5] = {0};
                memcpy(payload, (const uint8_t*)&altitude, 4);
                saveEventSimple(EVENT_LOW_ALTITUDE_DETECTED, payload);
                confRegister_.flightSubState = SUBSTATE_LANDING_VIDEO_SHORTSSTARTED;
                confRegister_.lastSubStateTime = uptime_ms;

                uint8_t i;
                for(i = 0; i < 4; i++)
                {
                    if((confRegister_.landing_camerasShort >> i) & 1)
                    {
                        cameraMakeVideo(i,
                                        CAMERAMODE_VID,
                                        confRegister_.landing_videoDurationShort);
                    }else if((confRegister_.landing_camerasHighSpeed >> i) & 1)
                    {
                        cameraMakeVideo(i,
                                        CAMERAMODE_VID,
                                        confRegister_.landing_videoDurationShort);
                    }
                }
            }
        }

        //Lets wait for the duration of the long video plus 30 minutes to go
        //to next step
        if(confRegister_.lastStateTime
                + (uint64_t)confRegister_.landing_videoDurationLong * 1000UL
                + 1800000UL
                < uptime_ms)
        {
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_TIMELAPSE_LAND;
            saveEventSimple(EVENT_STATE_CHANGED, payload);

            //Cameras are off, we move to the next step:
            confRegister_.flightState = FLIGHTSTATE_TIMELAPSE_LAND;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;

            //Activate interrupt for detecting the recovery team:
            i2c_ADXL345_activateActivityDetection();
        }
        else
        {
            if(landingDetectedFromSunrise_
                    && confRegister_.lastStateTime  + 600000 < uptime_ms)
            {
                int32_t currentAltitude = getAltitude();
                if(landingHeight_ - currentAltitude < 200000
                        && getBaroIsOnError() == 0)
                {
                    //we did less than 2000km in 10min, so interrupt
                    uint8_t payload[5] = {0};
                    payload[0] = FLIGHTSTATE_TIMELAPSE;
                    saveEventSimple(EVENT_STATE_CHANGED, payload);
                    //Cameras are off, we move to the next step:
                    confRegister_.flightState = FLIGHTSTATE_TIMELAPSE;
                    confRegister_.lastStateTime = uptime_ms;
                    //Reset the substate
                    confRegister_.flightSubState = 0;
                    confRegister_.lastSubStateTime = uptime_ms;

                    uint8_t i;
                    for(i = 0; i < 4; i++)
                    {
                        cameraInterruptVideo(i);
                    }
                }
            }
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

            lastTimePicture_ = uptime_s;
        }

        if(i2c_ADXL345_getMovementDetected() > 3) //More than 3 events in 1 minute
        {
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_RECOVERY;
            saveEventSimple(EVENT_STATE_CHANGED, payload);

            //we move to the next step:
            confRegister_.flightState = FLIGHTSTATE_RECOVERY;
            confRegister_.lastStateTime = uptime_ms;
            //Reset the substate
            confRegister_.flightSubState = 0;
            confRegister_.lastSubStateTime = uptime_ms;

            uint8_t i;
            for(i = 0; i < 4; i++)
            {
                cameraMakeVideo(i,
                                CAMERAMODE_VID,
                                confRegister_.recovery_videoDuration);
            }
        }
    }

    if(confRegister_.flightState == FLIGHTSTATE_RECOVERY)
    {
        //Remain here for ever
        uint32_t uptime_s = (uptime_ms / 1000UL);
        //Is it time to make another picture?
        if(lastTimePicture_
                + (uint32_t)confRegister_.flight_timelapse_period < uptime_s)
        {
            uint8_t i;

            for(i = 0; i < 4; i++)
            {
                //Switch on depending on when it is now:
                cameraTakePicture(i);
            }

            lastTimePicture_ = uptime_s;
        }
    }
}
