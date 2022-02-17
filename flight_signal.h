/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef FLIGHT_SIGNAL_H_
#define FLIGHT_SIGNAL_H_

#include <stdint.h>
#include <msp430.h>
#include "clock.h"

// Constants
#define TIME_BETWEEN_SIG    300     //[s] -> Time that has to pass after a signal ends
                                    // to consider that we have a new, different signal.
#define SIG_TIME            30     //[s] -> Time that a signal must be HIGH to be considered.


uint8_t sunrise_GPIO_Read();

#endif /* FLIGHT_SIGNAL_H_ */
