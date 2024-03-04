/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "i2c_DS1338Z.h"

//To keep the unixTime counting ourselves and not dealing with the RTC
//all the time:
struct RTCUnixtime unixTimeStatus_;

//Private functions:
uint32_t convert_to_unixTime(struct RTCDateTime dateTime);
//void convert_from_unixTime(uint32_t unixtime, struct RTCDateTime *dateTime);
uint8_t utils_time_getWeekdayFromDate(struct RTCDateTime *dateTime);

/**
 * Initializes the RTC
 */
int8_t i2c_RTC_init(void)
{
    //Read the RTC and sync with the uptime
    struct RTCDateTime dateFromRTC;
    i2c_RTC_getClockData(&dateFromRTC);
    unixTimeStatus_.uptime = seconds_uptime();
    unixTimeStatus_.unixtime = convert_to_unixTime(dateFromRTC);
    return 0;
}

/**
 * Sets the DateTime in the RTC
 */
int8_t i2c_RTC_setClockData(struct RTCDateTime *dateTime)
{
    uint8_t const rtcRegister = DS1338Z_SECONDS;

    uint8_t weekday = utils_time_getWeekdayFromDate(dateTime);
    uint8_t rtcValues[7] = {dateTime->seconds, dateTime->minutes, dateTime->hours, weekday, dateTime->date, dateTime->month, dateTime->year};

    uint8_t txBuffer[8];
    txBuffer[0] = rtcRegister;

    uint8_t i;
    for (i = 0; i < 7; i++) // Formatting of the date...
    {
        txBuffer[i+1] = ((((rtcValues[i] / 10) & 0x0F) << 4) | (rtcValues[i] % 10));
    }

    int8_t ack = i2c_write(I2C_BUS00, DS1338Z_ADDRESS, txBuffer, 8, 0);

    //Now save the event that has been put into time:

    confRegister_.rtcLastTimeUpdate = convert_to_unixTime(*dateTime);

    return ack;
}

/**
 * Gets the current DateTime from the RTC
 */
int8_t i2c_RTC_getClockDataRAW(struct RTCDateTime *dateTime)
{
    uint8_t rtcData[7];
    rtcData[0] = DS1338Z_SECONDS;
    int8_t ack = i2c_write(I2C_BUS00, DS1338Z_ADDRESS, rtcData, 1, 0);
    if (ack == 0)
        ack = i2c_requestFrom(I2C_BUS00, DS1338Z_ADDRESS, rtcData, 7, 0);

    if (ack == 0)
    {
        dateTime->seconds = ((uint8_t) ((rtcData[DS1338Z_SECONDS] & 0b01110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_SECONDS] & 0x0F));
        dateTime->minutes = ((uint8_t) ((rtcData[DS1338Z_MINUTES] & 0b01110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_MINUTES] & 0x0F));
        dateTime->hours =   ((uint8_t) ((rtcData[DS1338Z_HOURS] & 0b00110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_HOURS] & 0x0F));
        dateTime->date =    ((uint8_t) ((rtcData[DS1338Z_DATE] & 0b00110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_DATE] & 0x0F));
        dateTime->month =   ((uint8_t) ((rtcData[DS1338Z_MONTH] & 0b00010000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_MONTH] & 0x0F));
        dateTime->year =    ((uint8_t) ((rtcData[DS1338Z_YEAR] & 0b11110000) >> 4)) * 10
                + ((uint8_t) (rtcData[DS1338Z_YEAR] & 0x0F));
    }

    return ack;
}


/**
 * Gets the current DateTime from the RTC
 */
int8_t i2c_RTC_getClockData(struct RTCDateTime *dateTime)
{
    //Read from the i2c:
    int8_t i2cAck = i2c_RTC_getClockDataRAW(dateTime);

    if(confRegister_.rtcDriftFlag == 0 || confRegister_.rtcLastTimeUpdate == 0)
        return i2cAck;  //rtc drift disabled so just use the RTC data

    //Drift enabled, lets calculate the delta since date was saved:
    uint32_t unixtime = convert_to_unixTime(*dateTime);
    int32_t deltaTime = unixtime - confRegister_.rtcLastTimeUpdate;

    //Sanity check
    if(deltaTime < 0)
    {
        //This should have never happened
        confRegister_.rtcLastTimeUpdate = unixtime;
        return 1;   //Return error that the rtc has been reset or something
    }

    //How many extra seconds do we need to adjust?
    int32_t extraSeconds = deltaTime / confRegister_.rtcDrift;
    uint32_t newUnixTime = 0;
    if(confRegister_.rtcDriftFlag == 1)
        newUnixTime = unixtime + extraSeconds;
    else
        newUnixTime = unixtime - extraSeconds;

    //Now convert back the date to the original format
    convert_from_unixTime(newUnixTime, dateTime);

    return i2cAck;
}

/**
 * It returns the current unixt time. Every RTC_READ_PERIOD it will ask the RTC
 * for the correct time just in case.
 */
uint32_t i2c_RTC_unixTime_now()
{
    uint32_t now = seconds_uptime();
    if(unixTimeStatus_.uptime + RTC_READ_PERIOD < now || unixTimeStatus_.uptime == 0)
    {
        //Ask again the time
        struct RTCDateTime dateFromRTC;
        int8_t error = i2c_RTC_getClockData(&dateFromRTC);
        if(error == 0)
        {
            unixTimeStatus_.uptime = now;
            unixTimeStatus_.unixtime = convert_to_unixTime(dateFromRTC);
        }
        else
        {
            //uart_print(UART_DEBUG, "ERROR\r\n");
            //Indicate error on RTC i2c
            //TODO
        }
    }
    return unixTimeStatus_.unixtime + (now - unixTimeStatus_.uptime);

}

/**
 * It sets the current time on the RTC using the unixtime
 */
int8_t i2c_RTC_set_unixTime(uint32_t unixtime)
{
    struct RTCDateTime dateTime;
    convert_from_unixTime(unixtime, &dateTime);
    int8_t returnValue = i2c_RTC_setClockData(&dateTime);
    uint32_t now = seconds_uptime();
    unixTimeStatus_.uptime = now;
    unixTimeStatus_.unixtime = unixtime;
    return returnValue;
}

///////////////////////////////////////////////////////////////////////////////
// Functions to handle Unix Time format times with human readable versions

//Leap year calculator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

/**
 * Calculates the Unixtime of the selected date
 * Using the 32bit means that it will work only until the 19 January 2038 :-O
 */
uint32_t convert_to_unixTime(struct RTCDateTime dateTime)
{
    // assemble time elements into unsigned long

    if(dateTime.year > 69)
        dateTime.year = dateTime.year - 70;
    else
        dateTime.year = dateTime.year + 30;

    int i;
    uint32_t seconds;
    // seconds from 1970 till 1 jan 00:00:00 of the given year
    seconds = dateTime.year * (SECS_PER_DAY * 365);
    for (i = 0; i < dateTime.year; i++)
    {
        if (LEAP_YEAR(i))
            seconds +=  SECS_PER_DAY;   // add extra days for leap years
    }

    // add days for this year, months start from 1
    for (i = 1; i < dateTime.month; i++)
    {
        if ( (i == 2) && LEAP_YEAR(dateTime.year))
            seconds += SECS_PER_DAY * 29;
        else
            seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
    }

    seconds += (dateTime.date - 1) * SECS_PER_DAY;
    seconds += dateTime.hours * SECS_PER_HOUR;
    seconds += dateTime.minutes * SECS_PER_MIN;
    seconds += dateTime.seconds;

    return seconds;
}


/*
 * Break the given unixtime into time components
 * This is a more compact version of the C library localtime function
 * Note that year is offset from 1970 !!!
 * Source: http://forum.arduino.cc/index.php?topic=56310.5
 */
void convert_from_unixTime(uint32_t unixtime, struct RTCDateTime *dateTime)
{
    uint8_t monthLength;

    dateTime->seconds = unixtime % 60;
    unixtime /= 60; // now it is minutes
    dateTime->minutes = unixtime % 60;
    unixtime /= 60; // now it is hours
    dateTime->hours = unixtime % 24;
    unixtime /= 24; // now it is days
    //uint8_t Wday = ((unixtime + 4) % 7) + 1;  // Sunday is day 1

    uint8_t years = 0;
    uint32_t days = 0;
    while((unsigned)(days += (LEAP_YEAR(years) ? 366 : 365)) <= unixtime)
    {
        years++;
    }
    // years is offset from 1970

    days -= LEAP_YEAR(years) ? 366 : 365;
    unixtime -= days; // now it is days in this year, starting at 0

    days = 0;
    dateTime->month = 0;
    monthLength = 0;
    for (dateTime->month = 0; dateTime->month < 12; dateTime->month = dateTime->month + 1)
    {
        if (dateTime->month == 1)
        {
            // February
            if (LEAP_YEAR(years))
                monthLength = 29;
            else
                monthLength = 28;
        }
        else
            monthLength = monthDays[dateTime->month];

        if (unixtime >= monthLength)
            unixtime -= monthLength;
        else
            break;
    }
    if(years < 30)
        dateTime->year = years + 70;
    else
        dateTime->year = years - 30;

    dateTime->month = dateTime->month + 1;  // jan is month 1
    dateTime->date = unixtime + 1;     // day of month
}

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
