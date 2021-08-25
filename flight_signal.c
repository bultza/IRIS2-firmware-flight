/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */


#include "flight_signal.h"

uint8_t signalOngoing = 0;
uint32_t timeSignalStarted = 0;
uint32_t timeSignalEnded = 0;

uint32_t lastSignalExpired = 0;

// Public functions
uint8_t checkFlightSignal()
{
    // Retrieve state of P2.2 --> 1 if HIGH signal, 0 otherwise
    uint8_t actualSignalValue = (P2IN & 0x04);

    // A HIGH signal has been detected which was not ongoing
    if (signalOngoing == 0 && actualSignalValue == 1)
    {
        // Check that signal is well separated from last one
        // to avoid new signal triggers due to small variations.
        if (seconds_uptime() - lastSignalExpired > TIME_BETWEEN_SIG)
        {
            signalOngoing = 1;
            timeSignalStarted = seconds_uptime();
        }
    }
    // A HIGH signal was ongoing but has now ended
    else if (signalOngoing == 1 && actualSignalValue == 0)
    {
        signalOngoing = 0;
        timeSignalEnded = seconds_uptime();
        lastSignalExpired = timeSignalEnded;

        // If the HIGH signal remained for at least SIG_TIME s
        if (timeSignalEnded-timeSignalStarted >= SIG_TIME)
        {
            return 1;
        }

        signalOngoing = 0;
        timeSignalStarted = 0;
        timeSignalEnded = 0;
    }

    return 0;
}
