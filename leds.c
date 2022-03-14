/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2022
 */

#include "leds.h"


#define LED_ON          (P1OUT |= BIT0)
#define LED_OFF         (P1OUT &= ~BIT0)
#define LED_TOGGLE      (P1OUT ^= BIT0)

#define LED_B_OFF       (P3OUT |= BIT6)
#define LED_B_ON        (P3OUT &= ~BIT6)
#define LED_B_TOGGLE    (P3OUT ^= BIT6)

#define LED_G_OFF       (P3OUT |= BIT5)
#define LED_G_ON        (P3OUT &= ~BIT5)
#define LED_G_TOGGLE    (P3OUT ^= BIT5)

#define LED_R_OFF       (P3OUT |= BIT4)
#define LED_R_ON        (P3OUT &= ~BIT4)
#define LED_R_TOGGLE    (P3OUT ^= BIT4)

/**
 * To work only if configured
 */
void led_on()
{
    //if(confRegister_.leds)
        LED_ON;
}

/**
 * To work only if configured
 */
void led_off()
{
    LED_OFF;
}

/**
 * To work only if configured
 */
void led_toggle()
{
    //if(confRegister_.leds)
        LED_TOGGLE;
}


/**
 * To work only if configured
 */
void led_b_on()
{
    if(confRegister_.leds)
        LED_B_ON;
}

/**
 * To work only if configured
 */
void led_b_off()
{
    LED_B_OFF;
}

/**
 * To work only if configured
 */
void led_b_toggle()
{
    if(confRegister_.leds)
        LED_B_TOGGLE;
}

/**
 * To work only if configured
 */
void led_r_on()
{
    if(confRegister_.leds)
        LED_R_ON;
}

/**
 * To work only if configured
 */
void led_r_off()
{
        LED_R_OFF;
}

/**
 * To work only if configured
 */
void led_r_toggle()
{
    if(confRegister_.leds)
        LED_R_TOGGLE;
}


/**
 * To work only if configured
 */
void led_g_on()
{
    if(confRegister_.leds)
        LED_G_ON;
}

/**
 * To work only if configured
 */
void led_g_off()
{
    LED_G_OFF;
}

/**
 * To work only if configured
 */
void led_g_toggle()
{
    if(confRegister_.leds)
        LED_G_TOGGLE;
}
