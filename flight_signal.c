/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */


#include "flight_signal.h"

/**
 * It returns 1 if GPIO is high, 0 if is low.
 * Remember, if no signal, the GPIO reads HIGH (pull up configured). On
 * external signal we get LOW value
 */
uint8_t sunrise_GPIO_Read()
{
    if(P2IN & BIT2)
        return 1;
    else
        return 0;
}
