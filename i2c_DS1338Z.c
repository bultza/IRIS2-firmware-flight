/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "i2c_DS1338Z.h"

/**
 * Add description
 */
int8_t i2c_DS1338Z_getClockData(struct RTCDateTime *dateTime)
{
    uint8_t rtcData[7];
    rtcData[0] = DS1338Z_SECONDS;
    //uint8_t rtcRegisters[6] = {DS1338Z_SECONDS, DS1338Z_MINUTES, DS1338Z_HOURS, DS1338Z_DATE, DS1338Z_MONTH, DS1338Z_YEAR};
    int8_t ack = i2c_write(I2C_BUS00, DS1338Z_ADDRESS, rtcData, 1, 0);
    if (ack == 0)
        ack = i2c_requestFrom(I2C_BUS00, DS1338Z_ADDRESS, rtcData, 7, 0);

    if (ack == 0)
    {
        dateTime->seconds = ((uint8_t) ((rtcData[DS1338Z_SECONDS] & 0b01110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_SECONDS] & 0x0F));
        dateTime->minutes = ((uint8_t) ((rtcData[DS1338Z_MINUTES] & 0b01110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_MINUTES] & 0x0F));
        dateTime->hours =   ((uint8_t) ((rtcData[DS1338Z_HOURS] & 0b00100000) >> 4)) * 20
                + ((uint8_t) ((rtcData[DS1338Z_HOURS] & 0b00010000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_HOURS] & 0x0F));
        dateTime->date =    ((uint8_t) ((rtcData[DS1338Z_DATE] & 0b00110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_DATE] & 0x0F));
        dateTime->month =   ((uint8_t) ((rtcData[DS1338Z_MONTH] & 0b00010000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_MONTH] & 0x0F));
        dateTime->year =    ((uint8_t) ((rtcData[DS1338Z_YEAR] & 0b11110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_YEAR] & 0x0F));
    }
    else
    {
        dateTime->year = 0;
    }

    return 0;
}

/**
 * Set Time to the RTC
 */
int8_t i2c_DS1338Z_setClockData(struct RTCDateTime *dateTime)
{
    //TODO Change from fixed value to the configured value
    uint8_t buffer[8] = {0x00, 0x00, 0x43, 0x00, 0x06, 0x24, 0x07, 0x21};
    int8_t ack = i2c_write(I2C_BUS00, DS1338Z_ADDRESS, buffer, 8, 0);
    return ack;
}
