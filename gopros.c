/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "gopros.h"

struct CameraStatus cameraStatus_[4] = {0};
uint8_t cameraHasStarted[4] = {0};

/*
 * Begins an UART communication with the selected camera.
 */
int8_t gopros_cameraInit(uint8_t selectedCamera)
{

    //if (!cameraHasStarted[selectedCamera])
    //{
        uart_init(selectedCamera + 1, BR_57600);
        cameraHasStarted[selectedCamera] = 1;
    //}

    return 0;
}

/*
 * Sends power to camera and boots it. UART communication not needed.
 */
int8_t gopros_cameraRawPowerOn(uint8_t selectedCamera)
{
    if (selectedCamera == CAMERA01)
    {
        // INITIAL STATE
        CAMERA01_ON;

        P7DIR &= ~BIT5;      // Define button as input - high impedance
        sleep_ms(CAM_WAIT_POWER);

        // PRESS BUTTON
        P7OUT &= ~BIT5;     // Put output bit to 0
        P7DIR |= BIT5;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P7DIR &= ~BIT5;      // Define button as input - high impedance
    }
    else if(selectedCamera == CAMERA02)
    {
        // INITIAL STATE
        CAMERA02_ON;

        P7DIR &= ~BIT6;      // Define button as input - high impedance
        sleep_ms(CAM_WAIT_POWER);

        // PRESS BUTTON
        P7OUT &= ~BIT6;     // Put output bit to 0
        P7DIR |= BIT6;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P7DIR &= ~BIT6;      // Define button as input - high impedance

    }
    else if(selectedCamera == CAMERA03)
    {
        // INITIAL STATE
        CAMERA03_ON;

        P7DIR &= ~BIT7;      // Define button as input - high impedance
        sleep_ms(CAM_WAIT_POWER);

        // PRESS BUTTON
        P7OUT &= ~BIT7;     // Put output bit to 0
        P7DIR |= BIT7;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P7DIR &= ~BIT7;      // Define button as input - high impedance
    }
    else if(selectedCamera == CAMERA04)
    {
        // INITIAL STATE
        CAMERA04_ON;

        P5DIR &= ~BIT6;      // Define button as input - high impedance
        sleep_ms(CAM_WAIT_POWER);

        // PRESS BUTTON
        P5OUT &= ~BIT6;     // Put output bit to 0
        P5DIR |= BIT6;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P5DIR &= ~BIT6;      // Define button as input - high impedance
    }

    sleep_ms(5000);

    // Never auto power down.
    uart_print(selectedCamera + 1, CAM_AUTO_POWER_DOWN_NEVER);
    sleep_ms(CAM_WAIT_CONF_CHANGE);

    // Set date and time in camera.
    struct RTCDateTime dateTime;
    char * dateTimePLD = CAM_PAYLOAD_SET_DATETIME;
    char dateTimeCmd[40];
    i2c_RTC_getClockData(&dateTime);
    sprintf(dateTimeCmd, "%s %2d %2d %2d %2d %2d %2d\n", dateTimePLD,
            dateTime.year, dateTime.month, dateTime.date,
            dateTime.hours, dateTime.minutes, dateTime.seconds);
    uart_print(selectedCamera + 1, dateTimeCmd);

    return 0;
}

/*
 * Sends power off command and removes power. UART communication required.
 */
int8_t gopros_cameraRawSafePowerOff(uint8_t selectedCamera)
{
    //Send the command to power off
    uart_print(selectedCamera + 1, CAM_POWEROFF);
    sleep_ms(CAM_WAIT_POWER);

    //We need to close the UART otherwise we are losing power through
    //the opened UART port to the MewPro
    uart_close(selectedCamera + 1);

    if (selectedCamera == CAMERA01)
        CAMERA01_OFF;
    else if (selectedCamera == CAMERA02)
        CAMERA02_OFF;
    else if (selectedCamera == CAMERA03)
        CAMERA03_OFF;
    else if (selectedCamera == CAMERA04)
        CAMERA04_OFF;

    cameraHasStarted[selectedCamera] = 0;

    return 0;
}

/*
 * Takes a picture with selected camera.
 */
int8_t gopros_cameraRawTakePicture(uint8_t selectedCamera)
{
    uart_print(selectedCamera + 1, CAM_SET_PHOTO_MODE);
    sleep_ms(CAM_WAIT_CONF_CHANGE);
    uart_print(selectedCamera + 1, CAM_PHOTO_TAKE_PIC);

    return 0;
}

/*
 * Starts recording video with selected camera.
 */
int8_t gopros_cameraRawStartRecordingVideo(uint8_t selectedCamera)
{
    // Set default mode to video.
    uart_print(selectedCamera + 1, CAM_DEF_MODE_VIDEO);
    sleep_ms(CAM_WAIT_CONF_CHANGE);

    // Set video format to NTSC.
    uart_print(selectedCamera + 1, CAM_VIDEO_FORMAT_NTSC);
    sleep_ms(CAM_WAIT_CONF_CHANGE);

    // Set video resolution to 4K, FPS to 15, and FOV to Wide.
    char * videoPLD = CAM_PAYLOAD_VIDEO_RES_FPS_FOV;
    char * videoRes = CAM_VIDEO_RES_4K;
    char * videoFPS = CAM_VIDEO_FPS_15;
    char * videoFOV = CAM_VIDEO_FOV_WIDE;
    char videoDataCmd[28];
    sprintf(videoDataCmd, "%s %s %s %s\n", videoPLD, videoRes, videoFPS, videoFOV);
    uart_print(selectedCamera + 1, videoDataCmd);
    sleep_ms(CAM_WAIT_CONF_CHANGE);

    uart_print(selectedCamera + 1, CAM_SET_VIDEO_MODE);
    sleep_ms(CAM_WAIT_CONF_CHANGE);
    uart_print(selectedCamera + 1, CAM_VIDEO_START_REC);

    return 0;
}

/*
 * End video recording in selected camera.
 */
int8_t gopros_cameraRawStopRecordingVideo(uint8_t selectedCamera)
{
    uart_print(selectedCamera + 1, CAM_SET_VIDEO_MODE);
    sleep_ms(CAM_WAIT_CONF_CHANGE);
    uart_print(selectedCamera + 1, CAM_VIDEO_STOP_REC);

    return 0;
}

/*
 * Send a customised command to the selected camera.
 */
int8_t gopros_cameraRawSendCommand(uint8_t selectedCamera, char * cmd)
{
    uart_print(selectedCamera + 1, cmd);

    return 0;
}

/**
 * Press the power on button
 */
void pressButton(uint8_t camera)
{
    switch(camera)
    {
    case CAMERA01:
        // PRESS BUTTON
        P7OUT &= ~BIT5;     // Put output bit to 0
        P7DIR |= BIT5;      // Define button as output
        break;
    case CAMERA02:
        // PRESS BUTTON
        P7OUT &= ~BIT6;     // Put output bit to 0
        P7DIR |= BIT6;      // Define button as output
        break;
    case CAMERA03:
        // PRESS BUTTON
        P7OUT &= ~BIT7;     // Put output bit to 0
        P7DIR |= BIT7;      // Define button as output
        break;
    case CAMERA04:
        // PRESS BUTTON
        P5OUT &= ~BIT6;     // Put output bit to 0
        P5DIR |= BIT6;      // Define button as output
        break;
    }
}

/**
 * Release the power on button
 */
void releaseButton(uint8_t camera)
{
    switch(camera)
    {
    case CAMERA01:
        P7DIR &= ~BIT5;      // Define button as input - high impedance
        break;
    case CAMERA02:
        P7DIR &= ~BIT6;      // Define button as input - high impedance
        break;
    case CAMERA03:
        P7DIR &= ~BIT7;      // Define button as input - high impedance
        break;
    case CAMERA04:
        P5DIR &= ~BIT6;      // Define button as input - high impedance
        break;
    }
}

/**
 * interrupt power on camera
 */
void cutPower(uint8_t camera)
{
    //We need to close the UART otherwise we are losing power through
    //the opened UART port to the MewPro
    uart_close(camera + 1);
    switch(camera)
    {
    case CAMERA01:
        CAMERA01_OFF;
        break;
    case CAMERA02:
        CAMERA02_OFF;
        break;
    case CAMERA03:
        CAMERA03_OFF;
        break;
    case CAMERA04:
        CAMERA04_OFF;
        break;
    }
}

/**
 * Stars the Power On sequence
 */
int8_t cameraPowerOn(uint8_t selectedCamera)
{
    if(cameraStatus_[selectedCamera].cameraStatus > CAM_STATUS_OFF)
        return -1;  //Return error

    //Provide Power
    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_POWERON;
    switch(selectedCamera)
    {
    case CAMERA01:
        CAMERA01_ON;
        break;
    case CAMERA02:
        CAMERA02_ON;
        break;
    case CAMERA03:
        CAMERA03_ON;
        break;
    case CAMERA04:
        CAMERA04_ON;
        break;
    default:
        break;
    }
    //Init UART
    gopros_cameraInit(selectedCamera);
    //Start FSM
    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_POWERON_WAIT;
    cameraStatus_[selectedCamera].lastCommandTime = millis_uptime();
    cameraStatus_[selectedCamera].sleepTime = CAM_WAIT_POWER;

    return 0;
}

/**
 * Starts the Power Off sequence
 */
int8_t cameraPowerOff(uint8_t selectedCamera)
{
    if(cameraStatus_[selectedCamera].cameraStatus == CAM_STATUS_OFF)
        return -1;  //Return error

    //Send the command to power off
    uart_print(selectedCamera + 1, CAM_POWEROFF);

    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_PRESSBTNOFF;
    pressButton(selectedCamera);
    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_PRESSBTNOFF_WAIT;
    cameraStatus_[selectedCamera].lastCommandTime = millis_uptime();
    cameraStatus_[selectedCamera].sleepTime = CAM_WAIT_BUTTON + 1000;
    return 0;
}

/**
 * This function must be called continuously
 */
int8_t cameraFSMcheck()
{
    uint64_t uptime = millis_uptime();
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        if(cameraStatus_[i].fsmStatus == FSM_CAM_DONOTHING)
            continue;   //Camera is fine like it is :)
        if(cameraStatus_[i].lastCommandTime + cameraStatus_[i].sleepTime < uptime)
        {
            //Move to next step!
            switch(cameraStatus_[i].fsmStatus)
            {
            case FSM_CAM_POWERON_WAIT:
                pressButton(i);
                cameraStatus_[i].fsmStatus = FSM_CAM_PRESSBTN_WAIT;
                cameraStatus_[i].lastCommandTime = uptime;
                cameraStatus_[i].sleepTime = CAM_WAIT_BUTTON;
                break;
            case FSM_CAM_PRESSBTN_WAIT:
                releaseButton(i);
                cameraStatus_[i].fsmStatus = FSM_CAM_CONF_1;
                cameraStatus_[i].lastCommandTime = uptime;
                cameraStatus_[i].sleepTime = CAM_WAIT_CONF_CHANGE;
                break;
            case FSM_CAM_CONF_1:
                uart_print(i + 1, CAM_AUTO_POWER_DOWN_NEVER);
                cameraStatus_[i].fsmStatus = FSM_CAM_CONF_2;
                cameraStatus_[i].lastCommandTime = uptime;
                cameraStatus_[i].sleepTime = CAM_WAIT_CONF_CHANGE;
                break;
            case FSM_CAM_CONF_2:
                {
                    struct RTCDateTime dateTime;
                    char * dateTimePLD = CAM_PAYLOAD_SET_DATETIME;
                    char dateTimeCmd[40];
                    i2c_RTC_getClockData(&dateTime);
                    sprintf(dateTimeCmd, "%s 20 %2d %2d %2d %2d %2d %2d\n", dateTimePLD,
                            dateTime.year, dateTime.month, dateTime.date,
                            dateTime.hours, dateTime.minutes, dateTime.seconds);
                    uart_print(i + 1, dateTimeCmd);
                }
                cameraStatus_[i].fsmStatus = FSM_CAM_DONOTHING;
                cameraStatus_[i].cameraStatus = CAM_STATUS_ON;
                break;
            case FSM_CAM_PRESSBTNOFF_WAIT:
                releaseButton(i);
                cameraStatus_[i].fsmStatus = FSM_CAM_POWEROFF_WAIT;
                cameraStatus_[i].lastCommandTime = uptime;
                cameraStatus_[i].sleepTime = CAM_WAIT_BUTTON + 3000;
                break;
            case FSM_CAM_POWEROFF_WAIT:
                cutPower(i);
                cameraStatus_[i].fsmStatus = FSM_CAM_DONOTHING;
                cameraStatus_[i].cameraStatus = CAM_STATUS_OFF;
                break;
            }
        }
    }
    return 0;
}

