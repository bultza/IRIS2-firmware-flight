/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "utils_time.h"

/**
 * Gives the day of the week from 1 (Monday) to 7 (Sunday)
 */
uint8_t utils_time_getWeekdayFromDate(struct RTCDateTime *dateTime)
{
    uint8_t weekday;

    uint8_t d = dateTime->date;
    uint8_t m = dateTime->month;
    uint8_t y = 2000 + dateTime->year;

    weekday  = ((d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7) + 1;
    return weekday;
}
