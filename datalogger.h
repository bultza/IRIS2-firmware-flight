/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef DATALOGGER_H_
#define DATALOGGER_H_

#include <stdint.h>
#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "configuration.h"
#include "clock.h"
#include "i2c_MS5611.h"
#include "i2c_TMP75C.h"
#include "i2c_INA.h"
#include "i2c_ADXL345.h"
#include "i2c_DS1338Z.h"
#include "spi_NOR.h"
#include "flight_signal.h"

#define MEMORY_NOR      0
#define MEMORY_FRAM     1

#define TELEMETRYSAVEPERIOD     30  //[s]
#define NOR_TLM_ADDRESS         0
#define NOR_TLM_SIZE            0x03200000 // Last address of sector 199 is 0x031FFFFF
#define NOR_EVENTS_ADDRESS      0x03200000 // First address of sector 200
#define NOR_EVENTS_SIZE         0x00E00000
#define NOR_LAST_ADDRESS        0x03FFFFFF // per datasheet, page 45 (last address of sec. 255)

#define FRAM_TLM_ADDRESS        0x29FFC
#define FRAM_TLM_SIZE           0x19000  //this is 1600 telemetry lines at 64 bytes each
#define FRAM_EVENTS_ADDRESS     0x42FFC
#define FRAM_EVENTS_SIZE        0x00FFC  //this is 255 event lines at 16 bytes each

//Define all the events that we want to store on the memories
#define EVENT_BOOT                          69
#define EVENT_CONFIGURATION_CHANGED         1
#define EVENT_NOR_CLEAN                     2
#define EVENT_CAMERA_ON                     10
#define EVENT_CAMERA_PICTURE                11
#define EVENT_CAMERA_VIDEO_START            12
#define EVENT_CAMERA_VIDEO_END              13
#define EVENT_CAMERA_OFF                    14
#define EVENT_CAMERA_TIMELAPSE_PIC          15
#define EVENT_CAMERA_VIDEOMODE              16
#define EVENT_CAMERA_PICMODE                17
#define EVENT_I2C_ERROR_RESET               99
#define EVENT_SUNRISE_GPIO_CHANGE           100
#define EVENT_BATTERY_CUTOUT                200

#define AVG_INDEX               0
#define MAX_INDEX               1
#define MIN_INDEX               2

// ---NOR COMPUTATIONS---
// Mission: 10 days -> 10*24*60*60 = 864 000 seconds
// Proposing saving 1 Telemetry Line every 30 s --> 1.84 MB of telemetry
// (NOR memory has 64 MB)

// Proposed sector (256 kB, 500 pages)  partition:
// SECTOR 00 to SECTOR 199: Telemetry Lines (up to 51.2 MB of storage
//                          possible, 1.84 MB needed)
// SECTOR 200 to 255: Event Lines (up to 12.8 MB of storage possible)

// 64 Bytes per Telemetry Line
// 512 B per page, 64 B per line: 8 TM lines per page -->  4000 TM
//  lines per sector --> 800 000 TM lines possible
// (theoretically, about 30 days of telemetry can be saved)
#define TEMPERATURESENSORS_COUNT 3

struct TelemetryLine
{
    uint32_t unixTime;          // 4B - UNIX time [s]
    uint32_t upTime;            // 4B - Milliseconds since power on
                                //      (it will roll over after 49 days)
    uint32_t pressure;          // 4B - Atmospheric pressure [mb * 100]
    int32_t altitude;           // 4B - Altitude [cm]
    int16_t verticalSpeed[3];   // 2B x 3 - Vertical Speed [cm/s]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    int16_t temperatures[TEMPERATURESENSORS_COUNT];    // 2B X 5 = 10B: [C� * 10]
                                //      00 - PCB
                                //      01 - BARO
                                //      02 - External 01
                                //      03 - External 02
                                //      04 - External 03
    int16_t accXAxis[3];        // 2B x 3 - X-Axis acceleration [m/s]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    int16_t accYAxis[3];        // 2B x 3 - Y-Axis acceleration [m/s]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    int16_t accZAxis[3];        // 2B x 3 - Z-Axis acceleration [m/s]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    int16_t voltage[3];        // 2B x 3 - Battery voltages input [mV]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    int16_t current[3];        // 2B x 3 - Currents consumption [mA]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    uint16_t errors;            // 1B - BIT0: Camera 01 UART error
                                //      BIT1: Camera 02 UART error
                                //      BIT2: Camera 03 UART error
                                //      BIT3: Camera 04 UART error
                                //      BIT4: INA error
                                //      BIT5: BARO error
                                //      BIT6: RTC error
                                //      BIT7: Accelerometer error
                                //      BIT8: Temperature error
    uint8_t state;              // 1B - Current mission state
                                //      i.e. launch, making video
    uint8_t sub_state;          // 1B - Current mission sub-state.
                                //      i.e. launch, powering off camera 2
    uint8_t switches_status;    // 1B - BIT0: Camera 01
                                //      BIT1: Camera 02
                                //      BIT2: Camera 03
                                //      BIT3: Camera 04
                                //      BIT4: MUX Enable
                                //      BIT5: MUX Position
                                //      BIT6: Sunrise CMD
                                //      BIT7: Accelerometer Interrupt

    uint8_t padding[1];         // 1B - Padding to reach 64 bits
};

// ---NOR COMPUTATIONS---
// 16 Bytes per Event Line
// 512 B per page, 16 B per line: 32 EV lines per page --> 16000 EV lines
// per sector --> 896 000 EV lines possible
// (assuming 10 days trip, about 1 event per second can be saved)

struct EventLine
{
    uint32_t unixTime;          // 4B - UNIX time
    uint32_t upTime;            // 4B - Milliseconds since power on
                                //      (it will roll over after 49 days)
    uint8_t state;              // 1B - Actual Finite Machine State state
    uint8_t sub_state;          // 1B
    uint8_t event;              // 1B - Event code
    uint8_t payload[5];         // 5B - Extra information of the event
};

//Public function to return the addresses to continue writing in the NOR
void searchAddressesNOR();

//Public functions to read all sensors periodically and return TM Lines
void sensorsRead();
int8_t returnCurrentTMLines(struct TelemetryLine *tmLines);

//Public Functions to save data permanently
int8_t saveEvent(struct EventLine newEvent);
int8_t saveEventSimple(uint8_t code, uint8_t payload[5]);
int8_t saveTelemetry();

//Public Functions to get Saved data on the FRAM memory
int8_t addEventFRAM(struct EventLine newEvent, uint32_t *address);
int8_t getEventFRAM(uint16_t pointer, struct EventLine *savedEvent);

int8_t addTelemetryFRAM(struct TelemetryLine newTelemetry, uint32_t *address);
int8_t getTelemetryFRAM(uint16_t pointer, struct TelemetryLine *savedTelemetry);

//Public Functions to get Saved data on the NOR memory
int8_t addEventNOR(struct EventLine newEvent, uint32_t *address);
int8_t getEventNOR(uint32_t pointer, struct EventLine *savedEvent);

int8_t addTelemetryNOR(struct TelemetryLine *newTelemetry, uint32_t *address);
int8_t getTelemetryNOR(uint32_t pointer, struct TelemetryLine *savedTelemetry);

void printAltitudeHistory();
int32_t getVerticalSpeed();

#endif /* DATALOGGER_H_ */
