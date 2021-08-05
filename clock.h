/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 */

#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <msp430.h>
#include <stdbool.h>
#include <stdint.h>

// SMCLK frequency
#define CLOCK_FREQ      8u                         // Timer clock frequency (MHz)

#define DELAY_US(X)  (__delay_cycles(X*CLOCK_FREQ))

//Public functions
int8_t clock_init(void);
uint64_t millis_uptime(void);
uint32_t seconds_uptime(void);
void sleep_ms(const uint16_t ms);

#endif
