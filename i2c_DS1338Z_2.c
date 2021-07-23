/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include <i2c_DS1338Z_2.h>
#include <stdio.h>

int8_t i2c_DS1338Z_init(struct RTCDateTime *dateTime) // Initialize the RTC with DateTime values
{
    // Further configuration might be added here if deemed necessary

    return i2c_DS1338Z_setDateTime(dateTime);
}

int8_t i2c_DS1338Z_setDateTime(struct RTCDateTime *dateTime) // Initialize the RTC with DateTime values
{
    int8_t status = 0;
    uint8_t rtcRegisters[6] = {DS1338Z_SECONDS, DS1338Z_MINUTES, DS1338Z_HOURS, DS1338Z_DATE, DS1338Z_MONTH, DS1338Z_YEAR};
    uint8_t rtcValues[6] = {dateTime->seconds, dateTime->minutes, dateTime->hours, dateTime->date, dateTime->month, dateTime->year};

    uint8_t i;
    for (i = 0; i < 6; i = i + 1)
    {
        uint8_t datum;
        if (i == 0 | i == 1 | i == 5) // Formatting of Seconds, Minutes, Year
        {
            datum = ((((rtcValues[i] / 10) & 0x0F) << 4) | (rtcValues[i] % 10));
        }
        else if (i == 2) // Formatting of Hours, note: 24-hour format is enforced
        {
            datum = (((((rtcValues[i] / 20) & 0x0F) << 5) | (((rtcValues[i] / 10) & 0x0F) << 4) | (rtcValues[i] % 10))) | 0b00000000;
        }
        else if (i == 3) // Formatting of Date
        {
            datum = ((((rtcValues[i] / 10) &  0x0F) << 6) | (rtcValues[i] % 10));
        }
        else if (i == 4) // Formatting of Month
        {
            datum = ((((rtcValues[i] / 10) &  0x0F) << 7) | (rtcValues[i] % 10));
        }

        uint8_t setOfWords[2] = {rtcRegisters[i], datum};
        int8_t ack = i2c_write(I2C_BUS00, DS1338Z_ADDRESS, &setOfWords, 2, 0);

        if (ack != 0) // Failed setting some configuration
            status = -1;
    }

    return status;
}

int8_t i2c_DS1338Z_getDateTime(struct RTCDateTime *dateTime) // Read current DateTime value from RTC
{
    uint8_t rtcData[6];
    uint8_t rtcRegisters[6] = {DS1338Z_SECONDS, DS1338Z_MINUTES, DS1338Z_HOURS, DS1338Z_DATE, DS1338Z_MONTH, DS1338Z_YEAR};

    uint8_t i;
    for(i = 0; i < 6; i = i + 1)
    {
        uint8_t datum;
        int8_t ack = i2c_write(I2C_BUS00, DS1338Z_ADDRESS, &rtcRegisters[i], 1, 0);
        if (ack == 0)
            ack = i2c_requestFrom(I2C_BUS00, DS1338Z_ADDRESS, &datum, 1, 0);

        if (ack == 0)
        {
            if (i == 0 | i == 1) // Formatting of Seconds, Minutes
            {
                rtcData[i] = ((uint8_t) ((datum & 0b01110000) >> 4)) * 10 + ((uint8_t) (datum & 0x0F));
            }
            else if (i == 2) // Formatting of Hours
            {
                rtcData[i] = ((uint8_t) ((datum & 0b00100000) >> 4)) * 20 + ((uint8_t) ((datum & 0b00010000) >> 4)) * 10 + ((uint8_t) (datum & 0x0F));
            }
            else if (i == 3) // Formatting of Date
            {
                rtcData[i] = ((uint8_t) ((datum & 0b00110000) >> 4)) * 10 + ((uint8_t) (datum & 0x0F));
            }
            else if (i == 4) // Formatting of Month
            {
                rtcData[i] = ((uint8_t) ((datum & 0b00010000) >> 4)) * 10 + ((uint8_t) (datum & 0x0F));
            }
            else if (i == 5) // Formatting of Year
            {
                rtcData[i] = ((uint8_t) ((datum & 0b11110000) >> 4)) * 10 + ((uint8_t) (datum & 0x0F));
            }
        }
        else
            rtcData[i] = 32767; // Deterrent value
    }

    dateTime->seconds = rtcData[0];
    dateTime->minutes = rtcData[1];
    dateTime->hours = rtcData[2];
    dateTime->date = rtcData[3];
    dateTime->month = rtcData[4];
    dateTime->year = rtcData[5];

    return 0;
}
