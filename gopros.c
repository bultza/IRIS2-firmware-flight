/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "gopros.h"

/*
 * Sends power to camera and boots it
 */
uint8_t cameraRawPowerOn(uint8_t selectedCamera)
{
    if (selectedCamera == CAMERA01)
    {
        // INITIAL STATE
        CAMERA01_ON;
        sleep_ms(CAM_WAIT_POWER);
        P7DIR &= ~BIT5;      // Define button as input - high impedance

        // PRESS BUTTON
        P7OUT &= ~BIT5;     // Put output bit to 0
        P7DIR |= BIT5;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);     // Wait for 1 second

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P7DIR &= ~BIT5;      // Define button as input - high impedance
    }
    else if(selectedCamera == CAMERA02)
    {
        // INITIAL STATE
        CAMERA02_ON;
        sleep_ms(CAM_WAIT_POWER);
        P7DIR &= ~BIT6;      // Define button as input - high impedance

        // PRESS BUTTON
        P7OUT &= ~BIT6;     // Put output bit to 0
        P7DIR |= BIT6;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);     // Wait for 1 second

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P7DIR &= ~BIT6;      // Define button as input - high impedance

    }
    else if(selectedCamera == CAMERA03)
    {
        // INITIAL STATE
        CAMERA03_ON;
        sleep_ms(CAM_WAIT_POWER);
        P7DIR &= ~BIT7;      // Define button as input - high impedance

        // PRESS BUTTON
        P7OUT &= ~BIT7;     // Put output bit to 0
        P7DIR |= BIT7;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);     // Wait for 1 second

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P7DIR &= ~BIT7;      // Define button as input - high impedance
    }
    else if(selectedCamera == CAMERA04)
    {
        // INITIAL STATE
        CAMERA04_ON;
        sleep_ms(CAM_WAIT_POWER);
        P5DIR &= ~BIT6;      // Define button as input - high impedance

        // PRESS BUTTON
        P5OUT &= ~BIT6;     // Put output bit to 0
        P5DIR |= BIT6;      // Define button as output

        sleep_ms(CAM_WAIT_BUTTON);     // Wait for 1 second

        // RETURN TO INITIAL STATE - "UNPRESS BUTTON"
        P5DIR &= ~BIT6;      // Define button as input - high impedance
    }

    return 0;
}


