/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "gopros.h"

struct CameraStatus cameraStatus_[4] = {0};
uint8_t cameraHasStarted_[4] = {0};
uint8_t cameraMode_[4] = {CAMERAMODE_PIC};
char strToPrint_[50];

/*
 * Begins an UART communication with the selected camera.
 */
int8_t gopros_raw_cameraInit(uint8_t selectedCamera, uint8_t cameraMode)
{

    //if (!cameraHasStarted_[selectedCamera])
    //{
        uart_init(selectedCamera + 1, BR_57600);
        cameraHasStarted_[selectedCamera] = 1;
        cameraMode_[selectedCamera] = cameraMode;
    //}

    return 0;
}

/**
 * It sets the camera in Picture Mode
 */
int8_t gopros_raw_cameraSetPictureMode(uint8_t selectedCamera)
{
    if (!cameraHasStarted_[selectedCamera])
        return -1;
    uart_print(selectedCamera + 1, CAM_SET_PHOTO_MODE);
    uart_flush(selectedCamera + 1);
    return 0;
}

/**
 * It sets the camera in Video Mode
 */
int8_t gopros_raw_cameraSetVideoMode(uint8_t selectedCamera)
{
    if (!cameraHasStarted_[selectedCamera])
        return -1;

    if(confRegister_.gopro_model[selectedCamera] == 0)
    {
        uart_print(selectedCamera + 1, CAM_SET_VIDEO_MODE);
        uart_flush(selectedCamera + 1);
        sleep_ms(50);

        //GOPRO Hero4 Black
        uart_print(selectedCamera + 1, CAM_PAYLOAD_VIDEO_RES_FPS_FOV);
        uart_print(selectedCamera + 1, CAM_VIDEO_RES_2_7K_4_3);
        uart_print(selectedCamera + 1, CAM_VIDEO_FPS_30);
        uart_print(selectedCamera + 1, CAM_VIDEO_FOV_WIDE);
        //uart_print(selectedCamera + 1, CAM_VIDEO_RES_720);
        //uart_print(selectedCamera + 1, CAM_VIDEO_FPS_30);
        //uart_print(selectedCamera + 1, CAM_VIDEO_FOV_NARROW);
        uart_print(selectedCamera + 1, "\n");
        uart_flush(selectedCamera + 1);
        //TODO
    }
    else
    {
        uart_print(selectedCamera + 1, CAM_SET_VIDEO_MODE);
        uart_flush(selectedCamera + 1);
        sleep_ms(50);

        //GOPRO Hero4 Silver is less capable unfortunately
        uart_print(selectedCamera + 1, CAM_PAYLOAD_VIDEO_RES_FPS_FOV);
        uart_print(selectedCamera + 1, CAM_VIDEO_RES_1440);
        uart_print(selectedCamera + 1, CAM_VIDEO_FPS_48);
        uart_print(selectedCamera + 1, CAM_VIDEO_FOV_WIDE);
        uart_print(selectedCamera + 1, "\n");
        uart_flush(selectedCamera + 1);
    }

    return 0;
}

/*
 * It sends the command to take the picture
 */
int8_t gopros_raw_cameraTakePicture(uint8_t selectedCamera)
{
    if (!cameraHasStarted_[selectedCamera])
        return -1;
    uart_print(selectedCamera + 1, CAM_PHOTO_TAKE_PIC);
    uart_flush(selectedCamera + 1);
    if(confRegister_.debugUART == 5)
    {
        uint64_t uptime = millis_uptime();
        sprintf(strToPrint_, "%.3fs: Camera %d picture command.\r\n# ", uptime/1000.0, selectedCamera+1);
        uart_print(UART_DEBUG, strToPrint_);
    }
    return 0;
}


/*
 * Starts recording video with selected camera.
 */
int8_t gopros_raw_cameraStartRecordingVideo(uint8_t selectedCamera)
{
    if (!cameraHasStarted_[selectedCamera])
        return -1;
    uart_print(selectedCamera + 1, CAM_VIDEO_START_REC);
    uart_flush(selectedCamera + 1);
    return 0;
}

/*
 * End video recording in selected camera.
 */
int8_t gopros_raw_cameraStopRecordingVideo(uint8_t selectedCamera)
{
    if (!cameraHasStarted_[selectedCamera])
        return -1;
    uart_print(selectedCamera + 1, CAM_VIDEO_STOP_REC);
    uart_flush(selectedCamera + 1);
    return 0;
}

/**
 * Send command to format SD-Card
 */
int8_t gopros_raw_cameraFormatSDCard(uint8_t selectedCamera)
{
    if (!cameraHasStarted_[selectedCamera])
        return -1;
    uart_print(selectedCamera + 1, CAM_FORMAT_SDCARD);
    uart_flush(selectedCamera + 1);
    return 0;
}

/*
 * Send a customised command to the selected camera.
 */
int8_t gopros_raw_cameraRawSendCommand(uint8_t selectedCamera, char * cmd)
{
    if (!cameraHasStarted_[selectedCamera])
        return -1;
    uart_print(selectedCamera + 1, cmd);
    uart_flush(selectedCamera + 1);
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

    cameraHasStarted_[camera] = 0;
}

/**
 * Stars the Power On sequence
 */
int8_t cameraPowerOn(uint8_t selectedCamera, uint8_t slowMode)
{
    if(cameraStatus_[selectedCamera].cameraStatus > CAM_STATUS_OFF)
        return -1;  //Return error

    //Provide Power
    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_POWERON;
    cameraStatus_[selectedCamera].slowMode = slowMode;
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
    //Start FSM
    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_POWERON_WAIT;
    cameraStatus_[selectedCamera].lastCommandTime = millis_uptime();

    if(cameraStatus_[selectedCamera].slowMode)
        cameraStatus_[selectedCamera].sleepTime = 2500;
    else
        cameraStatus_[selectedCamera].sleepTime = 1500;

    return 0;
}

/**
 * Starts the Power Off sequence
 */
int8_t cameraPowerOff(uint8_t selectedCamera)
{
    int8_t error = 0;
    if(cameraStatus_[selectedCamera].cameraStatus == CAM_STATUS_OFF)
        error = -1;  //Return error

    //Send the command to power off
    uart_print(selectedCamera + 1, CAM_POWEROFF);
    uart_flush(selectedCamera + 1);

    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_PRESSBTNOFF;
    pressButton(selectedCamera);
    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_PRESSBTNOFF_WAIT;
    cameraStatus_[selectedCamera].lastCommandTime = millis_uptime();
    cameraStatus_[selectedCamera].sleepTime = CAM_WAIT_BUTTON /*+ 1000*/;
    return error;
}

/**
 * You should never use this way as you might corrupt the SDCard!!!
 */
int8_t cameraPowerOffUnsafe(uint8_t selectedCamera)
{
    cutPower(selectedCamera);
    cameraStatus_[selectedCamera].fsmStatus = FSM_CAM_DONOTHING;
    cameraStatus_[selectedCamera].cameraStatus = CAM_STATUS_OFF;
    return 0;
}

/**
 * It searches a particular flag in order to tell if the camera successfully booted up
 */
int8_t searchInUart(uint8_t uartIndex, uint8_t flag)
{
    uint16_t nchars = uart_available(uartIndex + 1);
    if(nchars < 1)
        return 0;
    uint16_t i;
    for(i = 0; i < nchars; i++)
    {
        //Search the flag
        if(uart_read(uartIndex + 1) == flag)
            return 1;
    }
    return 0;
}

/**
 * It checks the low level tasks of the camera
 */
int8_t cameraFSMlowLevelCheck()
{
    uint64_t uptime = millis_uptime();
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        if(cameraStatus_[i].fsmStatus == FSM_CAM_DONOTHING)
        {
            //Flush anything that is on the UART:
            searchInUart(i, 0);
            //Camera is fine like it is :)
            continue;
        }
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
                if(confRegister_.debugUART == 5)
                {
                    uint64_t uptime = millis_uptime();
                    sprintf(strToPrint_, "%.3fs: Camera %d Start pressing button.\r\n# ", uptime/1000.0, i+1);
                    uart_print(UART_DEBUG, strToPrint_);
                }
                break;
            case FSM_CAM_PRESSBTN_WAIT:
                releaseButton(i);
                cameraStatus_[i].fsmStatus = FSM_CAM_CONF_1;
                cameraStatus_[i].lastCommandTime = uptime;
                //cameraStatus_[i].sleepTime = /*CAM_WAIT_POWER*/ 2500;
                if(cameraStatus_[i].slowMode)
                    cameraStatus_[i].sleepTime = 4000;
                else
                    cameraStatus_[i].sleepTime = 0; //No need to wait because we wait for the UART answers

                //It usually takes between 2.1 and 2.7s, more than that is a timeout
                cameraStatus_[i].timeoutTime = 3000;

                if(confRegister_.debugUART == 5)
                {
                    uint64_t uptime = millis_uptime();
                    sprintf(strToPrint_, "%.3fs: Camera %d Stop pressing button.\r\n# ", uptime/1000.0, i+1);
                    uart_print(UART_DEBUG, strToPrint_);
                }
                break;
            case FSM_CAM_CONF_1:
                {
                    //Time out?
                    if(cameraStatus_[i].lastCommandTime + cameraStatus_[i].timeoutTime < uptime)
                    {
                        uint64_t uptime = millis_uptime();
                        sprintf(strToPrint_, "%.3fs: Camera %d timeout :(\r\n# ", uptime/1000.0, i+1);
                        uart_print(UART_DEBUG, strToPrint_);
                        //It is timeout however we continue with the rest of the configuration as blind
                    }
                    //Wait for uart to answer:
                    else if(searchInUart(i, '@') == 0)
                        continue; //Continue to the next camera

                    //With 10ms delay we dont have collisions but conf not always arrives
                    //With 50m delay seems that it never loses the conf
                    if(cameraStatus_[i].slowMode)
                        sleep_ms(200);
                    else
                        sleep_ms(50);


                    //This works:
                    //YY00072100232016011100000001000000000000000000000000000000000001000007E50A14020A0B
                    struct RTCDateTime dateTime;
                    char dateTimeCmd[100];
                    i2c_RTC_getClockData(&dateTime);
                    //sprintf(dateTimeCmd, "YY000721002320160111000000010002000101000000000000000000000000010100%02X%02X%02X%02X%02X%02X%02X\n",
                    sprintf(dateTimeCmd, "YY000721002320160111000000%02X00%02X00010100000000000000000000000001%02X00%02X%02X%02X%02X%02X%02X%02X\n",
                            confRegister_.gopro_beeps,
                            confRegister_.gopro_leds,
                            //cameraMode_[i],
                            1,  //Allways like camera mode
                            (uint8_t)((2000 + (uint16_t)dateTime.year)/256),
                            (uint8_t)((2000 + (uint16_t)dateTime.year)%256),
                            dateTime.month,
                            dateTime.date,
                            dateTime.hours,
                            dateTime.minutes,
                            dateTime.seconds);
                    uart_print(i + 1, dateTimeCmd);
                    uart_flush(i + 1);
                    if(confRegister_.debugUART == 5)
                    {
                        uint64_t uptime = millis_uptime();
                        sprintf(strToPrint_, "%.3fs: Camera %d conf: '", uptime/1000.0, i+1);
                        uart_print(UART_DEBUG, strToPrint_);
                        uart_print(UART_DEBUG, dateTimeCmd);
                        uart_print(UART_DEBUG, "'\r\n# ");
                    }
                }
                cameraStatus_[i].fsmStatus = FSM_CAM_CONF_2;
                cameraStatus_[i].lastCommandTime = uptime;
                if(cameraStatus_[i].slowMode)
                    cameraStatus_[i].sleepTime = 2000;
                else
                    cameraStatus_[i].sleepTime = 500;

                if(confRegister_.debugUART == 5)
                {
                    uint64_t uptime = millis_uptime();
                    sprintf(strToPrint_, "%.3fs: Camera %d is Configured.\r\n# ", uptime/1000.0, i+1);
                    uart_print(UART_DEBUG, strToPrint_);
                }
                break;
            case FSM_CAM_CONF_2:
                if(cameraMode_[i] == CAMERAMODE_VID)
                {
                    //Configure video:
                    //Depends of camera model:
                    if(confRegister_.gopro_model[i] == 0)
                    {
                        uart_print(i + 1, CAM_SET_VIDEO_MODE);
                        uart_flush(i + 1);
                        sleep_ms(200);

                        //GOPRO Hero4 Black
                        uart_print(i + 1, CAM_PAYLOAD_VIDEO_RES_FPS_FOV);
                        uart_print(i + 1, CAM_VIDEO_RES_2_7K_4_3);
                        uart_print(i + 1, CAM_VIDEO_FPS_30);
                        uart_print(i + 1, CAM_VIDEO_FOV_WIDE);
                        //uart_print(i + 1, CAM_VIDEO_RES_720);
                        //uart_print(i + 1, CAM_VIDEO_FPS_30);
                        //uart_print(i + 1, CAM_VIDEO_FOV_NARROW);
                        uart_print(i + 1, "\n");
                        uart_flush(i + 1);
                    }
                    else
                    {
                        uart_print(i + 1, CAM_SET_VIDEO_MODE);
                        uart_flush(i + 1);
                        sleep_ms(200);

                        //GOPRO Hero4 Silver is less capable unfortunately
                        uart_print(i + 1, CAM_PAYLOAD_VIDEO_RES_FPS_FOV);
                        uart_print(i + 1, CAM_VIDEO_RES_1440);
                        uart_print(i + 1, CAM_VIDEO_FPS_48);
                        uart_print(i + 1, CAM_VIDEO_FOV_WIDE);
                        uart_print(i + 1, "\n");
                        uart_flush(i + 1);
                    }

                }
                else if(cameraMode_[i] == CAMERAMODE_PIC)
                {
                    //Configure picture: easypeasy
                    uart_print(i + 1, CAM_SET_PHOTO_MODE);
                    uart_flush(i + 1);
                    if(cameraStatus_[i].slowMode)
                        sleep_ms(200);
                    else
                        sleep_ms(100);
                    uart_print(i + 1, CAM_PHOTO_RES_12MP_WIDE);
                    uart_flush(i + 1);
                }
                else if(cameraMode_[i] == CAMERAMODE_VID_HIGHSPEED)
                {
                    //Configure video in high speed mode:
                    uart_print(i + 1, CAM_SET_VIDEO_MODE);
                    uart_flush(i + 1);
                    sleep_ms(200);

                    //GOPRO Hero4 Black
                    uart_print(i + 1, CAM_PAYLOAD_VIDEO_RES_FPS_FOV);
                    uart_print(i + 1, CAM_VIDEO_RES_960);
                    uart_print(i + 1, CAM_VIDEO_FPS_120);
                    uart_print(i + 1, CAM_VIDEO_FOV_WIDE);
                    uart_print(i + 1, "\n");
                    uart_flush(i + 1);
                }

                cameraStatus_[i].fsmStatus = FSM_CAM_DONOTHING;
                cameraStatus_[i].cameraStatus = CAM_STATUS_ON;
                if(confRegister_.debugUART == 5)
                {
                    uint64_t uptime = millis_uptime();
                    sprintf(strToPrint_, "%.3fs: Camera %d is Ready.\r\n# ", uptime/1000.0, i+1);
                    uart_print(UART_DEBUG, strToPrint_);
                }
                break;
            case FSM_CAM_PRESSBTNOFF_WAIT:
                releaseButton(i);
                cameraStatus_[i].fsmStatus = FSM_CAM_POWEROFF_WAIT;
                cameraStatus_[i].lastCommandTime = uptime;
                if(cameraStatus_[i].slowMode)
                    cameraStatus_[i].sleepTime = 5000;
                else
                    cameraStatus_[i].sleepTime = 3500;
                break;
            case FSM_CAM_POWEROFF_WAIT:
                cutPower(i);
                cameraStatus_[i].fsmStatus = FSM_CAM_DONOTHING;
                cameraStatus_[i].cameraStatus = CAM_STATUS_OFF;
                if(confRegister_.debugUART == 5)
                {
                    uint64_t uptime = millis_uptime();
                    sprintf(strToPrint_, "%.3fs: Camera %d is Off.\r\n# ", uptime/1000.0, i+1);
                    uart_print(UART_DEBUG, strToPrint_);
                }
                break;
            }
        }
    }
    return 0;
}

/**
 * It checks the low level tasks of the camera
 */
int8_t cameraFSMhighLevelCheck()
{
    uint64_t uptime_ms = millis_uptime();
    //char strToPrint[50];
    uint8_t i;
    for(i = 0; i < 4; i++)
    {
        if(cameraStatus_[i].fsmStatusGlobal == FSMGLOBAL_CAM_DISABLED)
        {
            //Camera is fine like it is :)
            continue;
        }

        if(cameraStatus_[i].fsmStatusGlobalLastTime + cameraStatus_[i].fsmStatusGlobalsleepTime < uptime_ms)
        {
            //Move to next step!
            switch(cameraStatus_[i].fsmStatusGlobal)
            {
            case FSMGLOBAL_CAM_PICTURESTART:
                if(cameraStatus_[i].cameraStatus == CAM_STATUS_ON)
                {
                    cameraStatus_[i].fsmStatusGlobalLastTime = uptime_ms;
                    //Wait 1s for sending the picture command:
                    //cameraStatus_[i].fsmStatusGlobalsleepTime = 1500;
                    cameraStatus_[i].fsmStatusGlobalsleepTime = confRegister_.gopro_pictureSleep;
                    cameraStatus_[i].fsmStatusGlobal = FSMGLOBAL_CAM_PICTURESHOOT;
                }
                break;
            case FSMGLOBAL_CAM_PICTURESHOOT:
                gopros_raw_cameraTakePicture(i);
                cameraStatus_[i].fsmStatusGlobalLastTime = uptime_ms;
                //Wait 1s before sending the off command
                cameraStatus_[i].fsmStatusGlobalsleepTime = 1000;
                cameraStatus_[i].fsmStatusGlobal = FSMGLOBAL_CAM_OFF;
                break;
            case FSMGLOBAL_CAM_OFF:
                cameraPowerOff(i);
                cameraStatus_[i].fsmStatusGlobalLastTime = uptime_ms;
                cameraStatus_[i].fsmStatusGlobalsleepTime = 0;
                cameraStatus_[i].fsmStatusGlobal = FSMGLOBAL_CAM_DISABLED;
                break;
            case FSMGLOBAL_CAM_VIDEOSTART:
                gopros_raw_cameraStartRecordingVideo(i);
                cameraStatus_[i].fsmStatusGlobalLastTime = uptime_ms;
                //Wait 1s for sending the picture command:
                cameraStatus_[i].fsmStatusGlobalsleepTime = (uint64_t)cameraStatus_[i].videoDuration * 1000UL;
                cameraStatus_[i].fsmStatusGlobal = FSMGLOBAL_CAM_VIDEOSTOP;
                break;
            case FSMGLOBAL_CAM_VIDEOSTOP:
                gopros_raw_cameraStopRecordingVideo(i);
                cameraStatus_[i].fsmStatusGlobalLastTime = uptime_ms;
                //Wait 2s for sending the picture command:
                cameraStatus_[i].fsmStatusGlobalsleepTime = 2000;
                cameraStatus_[i].fsmStatusGlobal = FSMGLOBAL_CAM_OFF;
                {
                    uint8_t payload[5] = {0};
                    payload[0] = i;
                    saveEventSimple(EVENT_CAMERA_VIDEO_END, payload);
                }
                break;
            default:
                break;
            }
        }
    }
    return 0;
}

/**
 * This function must be called continuously
 */
int8_t cameraFSMcheck()
{
    int8_t result = 0;
    //Check low level stuff
    result += cameraFSMlowLevelCheck();
    //Check low level stuff
    result += cameraFSMhighLevelCheck();

    return result;
}

/**
 * Checks if all the cameras are ready for making a picture
 */
int8_t cameraReadyStatus()
{
    uint8_t i;
    uint8_t result = 0;
    for(i = 0; i < 4; i++)
    {
        uint8_t bit = 1 << i;
        if(cameraStatus_[i].cameraStatus == CAM_STATUS_ON)
            result |= bit;
        else
            result &= ~bit;
    }
    return result;
}

/**
 * It makes a video in a completely automatic way (from power on to power off)
 */
int8_t cameraMakeVideo(uint8_t selectedCamera, uint8_t cameraMode, uint16_t duration)
{
    if(cameraStatus_[selectedCamera].fsmStatusGlobal != FSMGLOBAL_CAM_DISABLED)
        return -1; //Camera was busy, return error!

    if(cameraStatus_[selectedCamera].fsmStatus != FSM_CAM_DONOTHING)
        return -2; //Camera was busy, return error!

    if(cameraStatus_[selectedCamera].cameraStatus != CAM_STATUS_OFF)
        return -3; //Camera was busy, return error!

    cameraStatus_[selectedCamera].fsmStatusGlobal = FSMGLOBAL_CAM_VIDEOSTART;
    cameraStatus_[selectedCamera].fsmStatusGlobalsleepTime = 15000;
    cameraStatus_[selectedCamera].fsmStatusGlobalLastTime = millis_uptime();
    cameraStatus_[selectedCamera].videoDuration = duration;

    uint8_t payload[5];
    payload[0] = selectedCamera;
    payload[1] = cameraMode;
    payload[2] = 0;
    payload[3] = (uint8_t) (0x00FF & (duration >> 8));
    payload[4] = (uint8_t) (0x00FF & duration);
    saveEventSimple(EVENT_CAMERA_VIDEO_START, payload);

    //Start switching on the camera
    gopros_raw_cameraInit(selectedCamera, cameraMode);
    return cameraPowerOn(selectedCamera, 1);    //Slow mode...
}

/**
 * It interrupts the current video in order to turn into a safe spot
 */
int8_t cameraInterruptVideo(uint8_t selectedCamera)
{
    if(cameraStatus_[selectedCamera].fsmStatusGlobal == FSMGLOBAL_CAM_VIDEOSTART
            || cameraStatus_[selectedCamera].fsmStatusGlobal == FSMGLOBAL_CAM_VIDEOSTOP)
    {
        //Make the video stop inmediately:
        cameraStatus_[selectedCamera].fsmStatusGlobal = FSMGLOBAL_CAM_VIDEOSTOP;
        cameraStatus_[selectedCamera].fsmStatusGlobalsleepTime = 0;
        cameraStatus_[selectedCamera].fsmStatusGlobalLastTime = millis_uptime();
        cameraStatus_[selectedCamera].videoDuration = 0;
        return 0;
    }

    uint8_t payload[5];
    payload[0] = selectedCamera;
    payload[1] = 0;
    payload[2] = 0;
    payload[3] = 0;
    payload[4] = 0;
    saveEventSimple(EVENT_CAMERA_VIDEO_INTERRUPT, payload);

    //Error the camera was not recording video (at least autonomously)
    return -1;
}

/**
 * It makes a picture in a completely automatic way (from power on to power off)
 */
int8_t cameraTakePicture(uint8_t selectedCamera)
{
    if(cameraStatus_[selectedCamera].fsmStatusGlobal != FSMGLOBAL_CAM_DISABLED)
        return -1; //Camera was busy, return error!

    if(cameraStatus_[selectedCamera].fsmStatus != FSM_CAM_DONOTHING)
        return -2; //Camera was busy, return error!

    if(cameraStatus_[selectedCamera].cameraStatus != CAM_STATUS_OFF)
        return -3; //Camera was busy, return error!

    cameraStatus_[selectedCamera].fsmStatusGlobal = FSMGLOBAL_CAM_PICTURESTART;
    cameraStatus_[selectedCamera].fsmStatusGlobalsleepTime = 0;
    cameraStatus_[selectedCamera].fsmStatusGlobalLastTime = millis_uptime();

    uint8_t payload[5];
    payload[0] = selectedCamera;
    payload[1] = 0;
    payload[2] = 0;
    payload[3] = 0;
    payload[4] = 0;
    saveEventSimple(EVENT_CAMERA_TIMELAPSE_PIC, payload);

    //Start switching on the camera
    gopros_raw_cameraInit(selectedCamera, CAMERAMODE_PIC);
    return cameraPowerOn(selectedCamera, 0);
}
