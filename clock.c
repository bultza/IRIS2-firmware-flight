#ifndef __CLOCK_C__
#define __CLOCK_C__

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include "clock.h"

//Number of seconds elapsed since boot
volatile uint32_t elapsedSeconds = 0;

#define CLOCK_LOCK_VALID_KEY    0xA5
#define CLOCK_LOCK_INVALID_KEY  0xBB // Any key except the valid one

// For the scheduler consumption:
static volatile bool clock_10ms_elapsed = false;
static volatile uint16_t clock_internal_count_value = 40000u;        // 40k = 10ms

// For time sync between 10ms timer and XTAL:
static volatile uint16_t clock_sync_measured_intosc_ticks = 0u;
static volatile bool clock_sync_measurement_ready = false;
static volatile uint8_t clock_sync_timer_a1_overflow_count = 0u;
//static uint8_t clock_sync_measurement_count = 0u;


//Private functions:
int8_t xtal_init();
void millis_init();

/**
 * Initialises the clock module, including subsystems.
 */
int8_t clock_init(void)
{
    int8_t l_ret = 0;

    l_ret = xtal_init();
    millis_init();
    return(l_ret);
}

/**
 * It starts the Low Frequency Xtal to 32kHz
 * It starts the VCO to 8MHz
 * It sets the MCK and SMCK to 8MHz and ACLK to 32kHz
 */
int8_t xtal_init()
{
    // Set up XT1 pins that are connected to PJ.4 and PJ.5
    PJSEL0 |= BIT4 | BIT5;

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Clock System Setup
    // Unlock CS registers
    CSCTL0_H = CSKEY_H;
    //Set DCO to 8MHz
    CSCTL1 = DCOFSEL_6;
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
    // Set all dividers
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1; // All dividers set to one, except for DIVS (SMCLK)
    //LFXT is on
    CSCTL4 &= ~LFXTOFF;

    uint8_t xtalFailure = 0;
    uint16_t counter = 0;

    do
    {
        // Clear XT1 fault flag
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
        //Delay to clear fault flag bits
        _delay_cycles(500000);
        counter++;
        if (counter > 10)
        {
            //Xtal failure!!!!
            //sup_status_p.xtal_error_count++;
            //Avoid dying here in a bootloop:
            break;
        }

    }
    //while (SFRIFG1 & OFIFG);        // Test oscillator fault flag
    while(CSCTL5 & LFXTOFFG);

    return xtalFailure;
}

//TEST
void xtal_check(void)
{
    // Clear XT1 fault flag
    CSCTL5 &= ~LFXTOFFG;
    SFRIFG1 &= ~OFIFG;
    //If oscillator fault flag set, change to internal clock
    if(SFRIFG1 & OFIFG)
    {
        // Clock System Setup
        // Unlock CS registers
        CSCTL0_H = CSKEY_H;
        CSCTL2 = SELA__LFXTCLK | SELS__LFXTCLK | SELM__LFXTCLK;
        //CSCTL2 = SELA__VLOCLK | SELM__DCOCLK | SELM__DCOCLK;
    }

}

/**
 * It sets Timer A3 to interrupt every 1s
 */
void millis_init(void)
{
    // TACCR0 interrupt enabled
    TA3CCTL0 = CCIE;
    //327.68 ~1ms
    TA3CCR0 = 32768;
    // ACLK, up mode
    TA3CTL = TASSEL__ACLK | MC__UP;

    //Simulate that we have an uptime of 45 days minus 10s
    //elapsedSeconds = 3887990UL;
}

/**
 * It returns the elapsed time in millis since millis_init was called
 * (hopefully since the MCU booted up).
 */
uint64_t millis_uptime(void)
{
    //Wait until we manage to read the value accurately (read twice the register
    //and it has the same number)

    //Also avoid taking the value when x == 32768 because the elapsed seconds become
    //unstable, maybe change before or after measuring x!!!
    volatile uint32_t x = 0;
    while (x != TA3R || x == 32768U || x == 32767U)
        x = TA3R;

    if (x > 32735U)
        return ((uint64_t)elapsedSeconds * 1000UL + 999UL);
    else
        return ((uint64_t)elapsedSeconds * 1000UL + ((uint32_t) (x) * 1000UL / 32768UL));
}

/**
 * Number of seconds since we booted up
 */
uint32_t seconds_uptime(void)
{
    return elapsedSeconds;
}

/**
 * It stops execution until time reached
 */
void sleep_ms(const uint8_t ms)
{
    uint32_t timeStart = millis_uptime();
    while(timeStart + ms > millis_uptime())
    {
        __no_operation();
    }
}


/**
 * This interrupt must happen only once per second
 */
#pragma vector=TIMER3_A0_VECTOR
__interrupt void TIMER_A3_ISR(void)
{
    // Timer increment
    elapsedSeconds++;
}


#endif /* __CLOCK_C__ */
