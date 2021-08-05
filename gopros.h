/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef GOPROS_H_
#define GOPROS_H_

#include <stdint.h>
#include <msp430.h>
#include "clock.h"

#define CAMERA01 1
#define CAMERA02 2
#define CAMERA03 3
#define CAMERA04 4

#define MUX_OFF         (P2OUT |=  BIT3)
#define CAMERA01_OFF    (P4OUT |=  BIT6)
#define CAMERA01_ON     (P4OUT &= ~BIT6)
#define CAMERA02_OFF    (P4OUT |=  BIT5)
#define CAMERA02_ON     (P4OUT &= ~BIT5)
#define CAMERA03_OFF    (P4OUT |=  BIT4)
#define CAMERA03_ON     (P4OUT &= ~BIT4)
#define CAMERA04_OFF    (P2OUT |=  BIT7)
#define CAMERA04_ON     (P2OUT &= ~BIT7)

// Functions
uint8_t cameraRawPowerOn(uint8_t selectedCamera);

#endif /* GOPROS_H_ */
