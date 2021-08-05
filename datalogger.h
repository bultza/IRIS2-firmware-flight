/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef DATALOGGER_H_
#define DATALOGGER_H_

#include <stdint.h>
#include <msp430.h>
#include "configuration.h"
#include "clock.h"

#define TELEMETRYSAVEPERIOD     30  //[s]
#define NOR_TELEMETRY_ADDRESS   0
#define NOR_EVENTS_ADDRESS      0x030D4000 //200*500*512 = 0x030D4000

#define FRAM_TLM_ADDRESS        0x29FFC
#define FRAM_TLM_SIZE           0x19000  //this is 1600 telemetry lines at 64 bytes each
#define FRAM_EVENTS_ADDRESS     0x42FFC
#define FRAM_EVENTS_SIZE        0x00FFC  //this is 255 event lines at 16 bytes each

//Define all the events that we want to store on the memories
#define EVENT_BOOT                          69
#define EVENT_CONFIGURATION_CHANGED         1
#define EVENT_NOR_CLEAN                     2


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
#define TEMPERATURESENSORS_COUNT 5

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
    int16_t temperatures[TEMPERATURESENSORS_COUNT];    // 2B X 5 = 10B: [Cº * 10]
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
    int16_t voltages[3];        // 2B x 3 - Battery voltages input [mV]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    int16_t currents[3];        // 2B x 3 - Currents consumption [mA]
                                //      00 - Average value
                                //      01 - Maximum value
                                //      02 - Minimum value
    uint8_t state;              // 1B - Current mission state
                                //      i.e. launch, making video
    uint8_t sub_state;          // 1B - Current mission sub-state.
                                //      i.e. launch, powering off camera 2
    uint8_t padding[4];         // 4B - Padding to reach 64 bits
};

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

//Public Functions to save data permanently
int8_t saveEvent(struct EventLine newEvent);
int8_t saveTelemetry(struct TelemetryLine newTelemetry);

//Public Functions to get Saved data on the FRAM memory
int8_t addEventFRAM(struct EventLine newEvent, uint32_t *address);
int8_t getEventFRAM(uint16_t pointer, struct EventLine *savedEvent);

int8_t addTelemetryFRAM(struct TelemetryLine newTelemetry, uint32_t *address);
int8_t getTelemetryFRAM(uint16_t pointer, struct TelemetryLine *savedTelemetry);

#endif /* DATALOGGER_H_ */
