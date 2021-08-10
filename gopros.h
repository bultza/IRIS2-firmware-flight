/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef GOPROS_H_
#define GOPROS_H_

#include <stdint.h>
#include <msp430.h>
#include <stdio.h>
#include "clock.h"
#include "uart.h"
#include "i2c_DS1338Z.h"

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
#define CAM_WAIT_BUTTON 50
#define CAM_WAIT_CONF_CHANGE 2000

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

/*
 * MewPro 2 commands to control GoPro HERO 4 Black
 */
// Camera general config
#define CAM_DEF_MODE_VIDEO "YY 00 07 0B 00"
#define CAM_DEF_MODE_PHOTO "YY 00 07 0B 01"
#define CAM_DEF_MODE_MULTISHOT "YY 00 07 0B 02"

#define CAM_VIDEO_FORMAT_NTSC "YY 00 07 13 00 01 00\n"
#define CAM_VIDEO_FORMAT_PAL "YY 00 07 13 00 01 01\n"

#define CAM_AUTO_POWER_DOWN_NEVER "YY 00 07 17 00 01 00\n"

#define CAM_PAYLOAD_SET_DATETIME "YY 00 07 1B 00 01";

// Set the mode of the camera
#define CAM_SET_VIDEO_MODE "YY 00 01 01 00 01 00\n"
#define CAM_SET_PHOTO_MODE "YY 00 01 01 00 01 01\n"

// Video mode: recording and settings
#define CAM_VIDEO_START_REC "YY 00 02 1B 00 00\n"
#define CAM_VIDEO_STOP_REC "YY 00 02 1C 00 00\n"

#define CAM_PAYLOAD_VIDEO_RES_FPS_FOV "YY 00 02 03 00 03"

#define CAM_VIDEO_RES_4K "01"
#define CAM_VIDEO_RES_4K_SUPERVIEW "02"
#define CAM_VIDEO_RES_2_7K "04"
#define CAM_VIDEO_RES_2_7K_SUPERVIEW "05"
#define CAM_VIDEO_RES_2_7K_4_3 "06"
#define CAM_VIDEO_RES_1440 "07"
#define CAM_VIDEO_RES_1080_SUPERVIEW "08"
#define CAM_VIDEO_RES_1080 "09"
#define CAM_VIDEO_RES_960 "0A"
#define CAM_VIDEO_RES_720_SUPERVIEW "0B"
#define CAM_VIDEO_RES_720 "0C"
#define CAM_VIDEO_RES_WVGA "0D"

#define CAM_VIDEO_FPS_240 "00"
#define CAM_VIDEO_FPS_120 "01"
#define CAM_VIDEO_FPS_100 "02"
#define CAM_VIDEO_FPS_90 "03"
#define CAM_VIDEO_FPS_80 "04"
#define CAM_VIDEO_FPS_60 "05"
#define CAM_VIDEO_FPS_50 "06"
#define CAM_VIDEO_FPS_48 "07"
#define CAM_VIDEO_FPS_30 "08"
#define CAM_VIDEO_FPS_25 "09"
#define CAM_VIDEO_FPS_24 "0A"
#define CAM_VIDEO_FPS_15 "0B"
#define CAM_VIDEO_FPS_12_5 "0C"

#define CAM_VIDEO_FOV_WIDE "00"
#define CAM_VIDEO_FOV_MEDIUM "01"
#define CAM_VIDEO_FOV_NARROW "02"
#define CAM_VIDEO_FOV_LINEAR "04"

#define CAM_VIDEO_EXP_AUTO "YY 00 02 28 00 01 00\n"
#define CAM_VIDEO_EXP_1_12_5 "YY 00 02 28 00 01 01"
#define CAM_VIDEO_EXP_1_15 "YY 00 02 28 00 01 02\n"
#define CAM_VIDEO_EXP_1_24 "YY 00 02 28 00 01 03\n"
#define CAM_VIDEO_EXP_1_25 "YY 00 02 28 00 01 04\n"
#define CAM_VIDEO_EXP_1_30 "YY 00 02 28 00 01 05\n"
#define CAM_VIDEO_EXP_1_48 "YY 00 02 28 00 01 06\n"
#define CAM_VIDEO_EXP_1_50 "YY 00 02 28 00 01 07\n"
#define CAM_VIDEO_EXP_1_60 "YY 00 02 28 00 01 08\n"
#define CAM_VIDEO_EXP_1_80 "YY 00 02 28 00 01 09\n"
#define CAM_VIDEO_EXP_1_90 "YY 00 02 28 00 01 0A\n"
#define CAM_VIDEO_EXP_1_96 "YY 00 02 28 00 01 0B\n"
#define CAM_VIDEO_EXP_1_100 "YY 00 02 28 00 01 0C\n"
#define CAM_VIDEO_EXP_1_120 "YY 00 02 28 00 01 0D\n"
#define CAM_VIDEO_EXP_1_160 "YY 00 02 28 00 01 0E\n"
#define CAM_VIDEO_EXP_1_180 "YY 00 02 28 00 01 0F\n"
#define CAM_VIDEO_EXP_1_192 "YY 00 02 28 00 01 10\n"
#define CAM_VIDEO_EXP_1_200 "YY 00 02 28 00 01 11\n"
#define CAM_VIDEO_EXP_1_240 "YY 00 02 28 00 01 12\n"
#define CAM_VIDEO_EXP_1_320 "YY 00 02 28 00 01 13\n"
#define CAM_VIDEO_EXP_1_360 "YY 00 02 28 00 01 14\n"
#define CAM_VIDEO_EXP_1_400 "YY 00 02 28 00 01 15\n"
#define CAM_VIDEO_EXP_1_480 "YY 00 02 28 00 01 16\n"
#define CAM_VIDEO_EXP_1_960 "YY 00 02 28 00 01 17\n"

// Camera mode: settings
#define CAM_PHOTO_TAKE_PIC "YY 00 03 17 00 00\n"

#define CAM_PHOTO_RES_12MP_WIDE "YY 00 03 03 00 01 00\n"
#define CAM_PHOTO_RES_7MP_WIDE "YY 00 03 03 00 01 01\n"
#define CAM_PHOTO_RES_7MP_MEDIUM "YY 00 03 03 00 01 02\n"
#define CAM_PHOTO_RES_5MP_MEDIUM "YY 00 03 03 00 01 03\n"

#define CAM_PHOTO_EXP_AUTO "YY 00 03 09 00 01 00\n"
#define CAM_PHOTO_EXP_2S "YY 00 03 09 00 01 01\n"
#define CAM_PHOTO_EXP_5S "YY 00 03 09 00 01 02\n"
#define CAM_PHOTO_EXP_10S "YY 00 03 09 00 01 03\n"
#define CAM_PHOTO_EXP_15S "YY 00 03 09 00 01 04\n"
#define CAM_PHOTO_EXP_20S "YY 00 03 09 00 01 05\n"
#define CAM_PHOTO_EXP_30S "YY 00 03 09 00 01 06\n"


// Safe camera power off
#define CAM_POWEROFF "ZZ 00 03 01 01\n"


// Functions
int8_t gopros_cameraInit(uint8_t selectedCamera);
int8_t gopros_cameraRawPowerOn(uint8_t selectedCamera);
int8_t gopros_cameraRawSafePowerOff(uint8_t selectedCamera);
int8_t gopros_cameraRawTakePicture(uint8_t selectedCamera);
int8_t gopros_cameraRawStartRecordingVideo(uint8_t selectedCamera);
int8_t gopros_cameraRawStopRecordingVideo(uint8_t selectedCamera);
int8_t gopros_cameraRawSendCommand(uint8_t selectedCamera, char * cmd);
int8_t cameraPowerOn(uint8_t selectedCamera);
int8_t cameraPowerOff(uint8_t selectedCamera);
int8_t cameraFSMcheck();

#endif /* GOPROS_H_ */
