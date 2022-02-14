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

#define CAMERAMODE_PIC  0
#define CAMERAMODE_VID  1

#define MUX_OFF         (P2OUT |=  BIT3)
#define CAMERA01_OFF    (P4OUT |=  BIT6)
#define CAMERA01_ON     (P4OUT &= ~BIT6)
#define CAMERA02_OFF    (P4OUT |=  BIT5)
#define CAMERA02_ON     (P4OUT &= ~BIT5)
#define CAMERA03_OFF    (P4OUT |=  BIT4)
#define CAMERA03_ON     (P4OUT &= ~BIT4)
#define CAMERA04_OFF    (P2OUT |=  BIT7)
#define CAMERA04_ON     (P2OUT &= ~BIT7)

#define CAM_WAIT_POWER          5000
#define CAM_WAIT_BUTTON         150
#define CAM_WAIT_CONF_CHANGE    3000

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
#define FSM_CAM_CONF_1              20
#define FSM_CAM_CONF_2              21
#define FSM_CAM_CONF_3              22
#define FSM_CAM_CONF_4              23
#define FSM_CAM_CONF_5              24

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
 * More info here:
 *  * https://orangkucing.github.io/Hero4_I2C_Commands.html
 *  * https://mewpro.cc/en/2017/02/25/list-of-gopro-hero-4-i2c-commands/
 */
// Camera general config
#define CAM_DEF_MODE_VIDEO          "YY00070B00\n"
#define CAM_DEF_MODE_PHOTO          "YY00070B01\n"
#define CAM_DEF_MODE_MULTISHOT      "YY00070B02\n"
#define CAM_VIDEO_FORMAT_NTSC       "YY000713000100\n"
#define CAM_VIDEO_FORMAT_PAL        "YY000713000101\n"
#define CAM_AUTO_POWER_DOWN_NEVER   "YY000717000100\n"
#define CAM_PAYLOAD_SET_DATETIME    "YY00071B0001";


/*
Example: //YY00072100232016011100000001000000000000000000000000000000000001000007E50A14020A0B
We like: //YY000721002320160111000000010002000101000000000000000000000000010100YYYYMMDDhhmmss
SET_CAMERA bulk set global settings; argc = 0x0023; argv[]
argv[0]: 0x20 (unknown constant)
argv[1]: 0x16 (unknown constant)
argv[2]: 0x01 (unknown constant)
argv[3]: 0x11 (unknown constant)
argv[4]: 0x00 (unknown constant)
argv[5]: 0x00 (unknown constant)
argv[6]: OSD                    00    00 = off, 01 = on
argv[7]: beeps                  01    01 = 70%, 02 = off
argv[8]: auto power off         00    00 = Never, 01 = 1min
argv[9]: LEDs                   02    00 = off, 01 = 2, 02 = 4
argv[10]: quick capture         00    00 = off
argv[11]: orientation           01    00 = auto, 01 = up, 02 = down
argv[12]: LCD brightness        01    00 = high, 01 = medium, 02 = low
argv[13]: LCD sleep             00    00 = never, 01 = 1 min, 03 = 3min
argv[14]: LCD lock              00    00 = off, 01 = on
argv[15]: LCD power             00    00 = off, 01 = on
argv[16]: video format          00    00 = NTSC, 01 = PAL
argv[17]: language              00    00 = English
argv[18:24]: (no use)
argv[25]: 0x01 (unknown constant)
argv[26]: default mode          00    00 = Video, 01 = Photo, 02 = Multishot
argv[27]: default submode       00    00 = Video, single, 01 = timelapse, continuous, 02 = photoinvideo, night
argv[28]: high byte of year
argv[29]: low byte of year
argv[30]: month
argv[31]: day
argv[32]: hour
argv[33]: minute
argv[34]: second
 */

// Set the mode of the camera
#define CAM_SET_VIDEO_MODE          "YY000101000100\n"
#define CAM_SET_PHOTO_MODE          "YY000101000101\n"

// Video mode: recording and settings
#define CAM_VIDEO_START_REC         "YY00021B0000\n"
#define CAM_VIDEO_STOP_REC          "YY00021C0000\n"
#define CAM_PAYLOAD_VIDEO_RES_FPS_FOV "YY0002030003"

#define CAM_VIDEO_RES_4K            "01"
#define CAM_VIDEO_RES_4K_SUPERVIEW  "02"
#define CAM_VIDEO_RES_2_7K          "04"
#define CAM_VIDEO_RES_2_7K_SUPERVIEW    "05"
#define CAM_VIDEO_RES_2_7K_4_3      "06"
#define CAM_VIDEO_RES_1440          "07"
#define CAM_VIDEO_RES_1080_SUPERVIEW    "08"
#define CAM_VIDEO_RES_1080          "09"
#define CAM_VIDEO_RES_960           "0A"
#define CAM_VIDEO_RES_720_SUPERVIEW "0B"
#define CAM_VIDEO_RES_720           "0C"
#define CAM_VIDEO_RES_WVGA          "0D"

#define CAM_VIDEO_FPS_240           "00"
#define CAM_VIDEO_FPS_120           "01"
#define CAM_VIDEO_FPS_100           "02"
#define CAM_VIDEO_FPS_90            "03"
#define CAM_VIDEO_FPS_80            "04"
#define CAM_VIDEO_FPS_60            "05"
#define CAM_VIDEO_FPS_50            "06"
#define CAM_VIDEO_FPS_48            "07"
#define CAM_VIDEO_FPS_30            "08"
#define CAM_VIDEO_FPS_25            "09"
#define CAM_VIDEO_FPS_24            "0A"
#define CAM_VIDEO_FPS_15            "0B"
#define CAM_VIDEO_FPS_12_5          "0C"

#define CAM_VIDEO_FOV_WIDE          "00"
#define CAM_VIDEO_FOV_MEDIUM        "01"
#define CAM_VIDEO_FOV_NARROW        "02"
#define CAM_VIDEO_FOV_LINEAR        "04"

#define CAM_VIDEO_EXP_AUTO          "YY000228000100\n"
#define CAM_VIDEO_EXP_1_12_5        "YY000228000101\n"
#define CAM_VIDEO_EXP_1_15          "YY000228000102\n"
#define CAM_VIDEO_EXP_1_24          "YY000228000103\n"
#define CAM_VIDEO_EXP_1_25          "YY000228000104\n"
#define CAM_VIDEO_EXP_1_30          "YY000228000105\n"
#define CAM_VIDEO_EXP_1_48          "YY000228000106\n"
#define CAM_VIDEO_EXP_1_50          "YY000228000107\n"
#define CAM_VIDEO_EXP_1_60          "YY000228000108\n"
#define CAM_VIDEO_EXP_1_80          "YY000228000109\n"
#define CAM_VIDEO_EXP_1_90          "YY00022800010A\n"
#define CAM_VIDEO_EXP_1_96          "YY00022800010B\n"
#define CAM_VIDEO_EXP_1_100         "YY00022800010C\n"
#define CAM_VIDEO_EXP_1_120         "YY00022800010D\n"
#define CAM_VIDEO_EXP_1_160         "YY00022800010E\n"
#define CAM_VIDEO_EXP_1_180         "YY00022800010F\n"
#define CAM_VIDEO_EXP_1_192         "YY000228000110\n"
#define CAM_VIDEO_EXP_1_200         "YY000228000111\n"
#define CAM_VIDEO_EXP_1_240         "YY000228000112\n"
#define CAM_VIDEO_EXP_1_320         "YY000228000113\n"
#define CAM_VIDEO_EXP_1_360         "YY000228000114\n"
#define CAM_VIDEO_EXP_1_400         "YY000228000115\n"
#define CAM_VIDEO_EXP_1_480         "YY000228000116\n"
#define CAM_VIDEO_EXP_1_960         "YY000228000117\n"

// Camera mode: settings
#define CAM_PHOTO_TAKE_PIC          "YY0003170000\n"
#define CAM_PHOTO_RES_12MP_WIDE     "YY000303000100\n"
#define CAM_PHOTO_RES_7MP_WIDE      "YY000303000101\n"
#define CAM_PHOTO_RES_7MP_MEDIUM    "YY000303000102\n"
#define CAM_PHOTO_RES_5MP_MEDIUM    "YY000303000103\n"
#define CAM_PHOTO_EXP_AUTO          "YY000309000100\n"
#define CAM_PHOTO_EXP_2S            "YY000309000101\n"
#define CAM_PHOTO_EXP_5S            "YY000309000102\n"
#define CAM_PHOTO_EXP_10S           "YY000309000103\n"
#define CAM_PHOTO_EXP_15S           "YY000309000104\n"
#define CAM_PHOTO_EXP_20S           "YY000309000105\n"
#define CAM_PHOTO_EXP_30S           "YY000309000106\n"

// Safe camera power off
#define CAM_POWEROFF                "ZZ000301\n"


// Functions
int8_t gopros_cameraInit(uint8_t selectedCamera, uint8_t cameraMode);
int8_t gopros_cameraSetPictureMode(uint8_t selectedCamera);
int8_t gopros_cameraSetVideoMode(uint8_t selectedCamera);
int8_t gopros_cameraTakePicture(uint8_t selectedCamera);
int8_t gopros_cameraStartRecordingVideo(uint8_t selectedCamera);
int8_t gopros_cameraStopRecordingVideo(uint8_t selectedCamera);
int8_t gopros_cameraRawSendCommand(uint8_t selectedCamera, char * cmd);
int8_t cameraPowerOn(uint8_t selectedCamera);
int8_t cameraPowerOff(uint8_t selectedCamera);
int8_t cameraTakePic(uint8_t selectedCamera);
int8_t cameraFSMcheck();
int8_t cameraReadyStatus();

#endif /* GOPROS_H_ */
