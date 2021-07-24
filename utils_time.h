/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef UTILS_TIME_H_
#define UTILS_TIME_H_


#include <msp430.h>
#include <stdint.h>
#include <i2c_DS1338Z.h>

uint8_t utils_time_getWeekdayFromDate(struct RTCDateTime *dateTime);

#endif /* UTILS_TIME_H_ */
