/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2022
 *
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <stdint.h>
#include <msp430.h>
#include <stdio.h>
#include "configuration.h"

void led_on();
void led_off();
void led_toggle();

void led_r_on();
void led_r_off();
void led_r_toggle();

void led_g_on();
void led_g_off();
void led_g_toggle();

void led_b_on();
void led_b_off();
void led_b_toggle();



#endif /* LEDS_H_ */
