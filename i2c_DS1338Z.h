/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef I2C_DS1338Z_H_
#define I2C_DS1338Z_H_


#include <msp430.h>
#include <stdint.h>
#include "i2c.h"
#include "clock.h"

#define DS1338Z_ADDRESS 0x68
#define DS1338Z_SECONDS 0x00
#define DS1338Z_MINUTES 0x01
#define DS1338Z_HOURS 0x02
#define DS1338Z_DAY 0x03
#define DS1338Z_DATE 0x04
#define DS1338Z_MONTH 0x05
#define DS1338Z_YEAR 0x06
#define DS1338Z_CONTROL 0x07

#define RTC_READ_PERIOD 300     //Sync with RTC every 300s


struct RTCDateTime
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t date;
    uint8_t month;
    uint8_t year;
};

struct RTCUnixtime
{
    uint32_t uptime;
    uint32_t unixtime;
};

int8_t   i2c_RTC_init(void);
int8_t   i2c_RTC_setClockData(struct RTCDateTime *dateTime);
int8_t   i2c_RTC_getClockData(struct RTCDateTime *dateTime);

uint32_t i2c_RTC_unixTime_now();
int8_t   i2c_RTC_set_unixTime(uint32_t unixtime);




#endif /* I2C_DS1338Z_H_ */
