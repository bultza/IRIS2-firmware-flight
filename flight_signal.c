/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "flight_signal.h"
uint32_t sunrise_timeSignalStarted_ = 0;
uint8_t  sunrise_gpioLastStatus_ = 1;   //Starts high due to the pull up
uint8_t  sunrise_signalStatus_ = 0;

/**
 * It returns 1 if GPIO is high, 0 if is low.
 * Remember, if no signal, the GPIO reads HIGH (pull up configured). On
 * external signal we get LOW value
 */
uint8_t sunrise_GPIO_Read_RAW_no()
{
    if(P2IN & BIT2)
        return 1;
    else
        return 0;
}

/**
 * It will return 1 if the signal has arrived for more than 15s and for 60s
 * then it will reset again
 */
uint8_t sunrise_GPIO_Read_Signal()
{
    return sunrise_signalStatus_;
}

/**
 * This function must be called continiously and it will trigger a signal
 * when the Sunrise signal is longer than 15s
 */
uint8_t checkFlightSignal()
{
    uint8_t currentSignalStatus = sunrise_GPIO_Read_RAW_no();
    uint32_t uptime_s = seconds_uptime();

    if(currentSignalStatus != sunrise_gpioLastStatus_)
    {
        uint8_t payload[5] = {0};
        payload[0] = currentSignalStatus;
        saveEventSimple(EVENT_SUNRISE_GPIO_CHANGE, payload);
    }

    //Save the status:
    sunrise_gpioLastStatus_ = currentSignalStatus;

    if(currentSignalStatus)
    {
        //Signal is HIGH, so no external signal detected refresh time:
        sunrise_timeSignalStarted_ = uptime_s;
        sunrise_signalStatus_ = 0;
        return 0;
    }

    //If execution reaches this point, we have an external signal!!

    //For more than 15s??
    if(sunrise_timeSignalStarted_ + SIG_TIME > uptime_s )
    {
        //Not yet, so ignore from here because we have to wait :)
        sunrise_signalStatus_ = 0;
        return 0;
    }

    if(sunrise_signalStatus_ == 0)
    {
        uint8_t payload[5] = {0};
        payload[0] = currentSignalStatus;
        saveEventSimple(EVENT_SUNRISE_SIGNAL_DETECTED, payload);
        if(confRegister_.debugUART == 5)
        {
            char strToPrint[70];
            uint64_t uptime = millis_uptime();
            sprintf(strToPrint, "%.3fs: Sunrise activation signal detected!\r\n# ", uptime/1000.0);
            uart_print(UART_DEBUG, strToPrint);
        }
    }

    //Ok, already more than 15s, awesome!
    sunrise_signalStatus_ = 1;

    return 1;
}
