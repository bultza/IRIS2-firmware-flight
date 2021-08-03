/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef DATALOGGER_H_
#define DATALOGGER_H_


// Mission: 10 days -> 10*24*60*60 = 864 000 seconds
// Proposing saving 1 Telemetry Line per second --> 27.648 MB of telemetry (NOR memory has 64 MB)

// Proposed sector (256 kB, 500 pages)  partition:
// SECTOR 00 to SECTOR 199: Telemetry Lines (up to 51.2 MB of storage possible, 27.7 MB needed)
// SECTOR 200 to 255: Event Lines (up to 12.8 MB of storage possible)

// 32 Bytes per Telemetry Line
// 512 B per page, 32 B per line: 16 TM lines per page -->  8000 TM lines per sector --> 1 600 000 TM lines possible
// (theoretically, up to 18.5 days of telemetry can be saved)

struct TelemetryLine
{
    uint32_t unixTime;          // 4B - UNIX time
    uint64_t upTime;            // 8B - Milliseconds since power on
    int16_t temperature;        // 2B - Temperature
    uint32_t pressure;          // 4B - Atmospheric pressure
    int32_t altitude;           // 4B - Altitude
    int16_t accXAxis;           // 2B - X-Axis acceleration
    int16_t accYAxis;           // 2B - Y-Axis acceleration
    int16_t accZAxis;           // 2B - Z-Axis acceleration
    int16_t voltage;            // 2B - Battery voltage input
    int16_t current;            // 2B - Current consumption


};

// 14 Bytes per Event Line
// 512 B per page, 16 B per line: 32 EV lines per page --> 16000 EV lines per sector --> 896 000 EV lines possible
// (assuming 10 days trip, about 1 event per second can be saved)

struct EventLine
{
    uint32_t unixTime;          // 4B - UNIX time
    uint64_t upTime;            // 8B - Milliseconds since power on
    uint8_t event;              // 1B - Event code
    uint8_t state;              // 1B - Actual Finite Machine State state
    uint16_t padding;           // 2B padding to round up to 16 B
};

#endif /* DATALOGGER_H_ */
