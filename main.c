/**
 * IRIS2 CPU Flight Firmware
 * ==============================
 *
 * The IRIS2 CPU is an on-board computer based on the MSP430 FRAM controller
 * It controls the power of 4 GoPros and commands them with UART and directly
 * operating the power button.
 *
 * Authors:
 *      Aitor Conde <aitorconde@gmail.com>
 *      Ram�n Garc�a <ramongarciaalarcia@gmail.com>
 *
 */

/**
 * MCU Pinout
 *
 *
 * P1.0  DEBUG_LED
 *
 * P2.2 SUNRISE_CMD
 * P2.3 UART_Mux_Enable
 * P2.4 UART_Mux_Selection
 * P2.7 5VPOWER_EN_4
 *
 * P3.7 ACC_INT
 *
 * P4.0 POWERCAM01_STATUS
 * P4.1 POWERCAM02_STATUS
 * P4.2 POWERCAM03_STATUS
 * P4.4 5VPOWER_EN_3
 * P4.5 5VPOWER_EN_2
 * P4.6 5VPOWER_EN_1
 *
 *
 * P5.6 POWERCAM04_BUTTON
 * P5.7 POWERCAM04_STATUS
 *
 * P7.5 POWERCAM01_BUTTON
 * P7.6 POWERCAM02_BUTTON
 * P7.7 POWERCAM03_BUTTON
 *
 * P8.1 LED_R
 * P8.2 LED_G
 * P8.3 LED_B
 *
 *
 * UCA0TXD  UCA0TXD     //External UART
 * UCA0RXD  UCA0RXD     //External UART
 *
 * UCA1TXD  UCA1TXD     //CAMERA01 UART
 * UCA1RXD  UCA1RXD     //CAMERA01 UART
 *
 * UCA2TXD  UCA2TXD     //CAMERA02 UART
 * UCA2RXD  UCA2RXD     //CAMERA02 UART
 *
 *
 * UCA3TXD  UCA3TXD     //CAMERA03 and CAMERA04 UART
 * UCA3RXD  UCA3RXD     //CAMERA03 and CAMERA04 UART
 *
 * UCB0SDA  UCB0SDA     //I2C internal sensors
 * UCB0SCL  UCB0SCL     //I2C internal sensors
 *
 * P5.3 FLASH_CS1       //NOR Flash
 * P3.6 FLASH_CS2       //NOR Flash
 * UCB1SIMO UCB1SIMO    //NOR Flash
 * UCB1SOMI UCB1SOMI    //NOR Flash
 * UCB1CLK  UCB1CLK
 *
 * UCB2SDA  UCB2SDA     //I2C external sensors
 * UCB2SCL  UCB2SCL     //I2C external sensors
 *
 * LFXIN    LFXIN
 * LFXOUT   LFXOUT
 *
 */

#include <msp430.h> 
#include <stdint.h>
#include "uart.h"
#include "clock.h"
//#include "i2c.h"

#define LED_ON          (P1OUT |= BIT0)
#define LED_OFF         (P1OUT &= ~BIT0)
#define LED_TOGGLE      (P1OUT ^= BIT0)

#define MUX_OFF         (P2OUT |=  BIT3)

#define CAMERA01_OFF    (P4OUT |=  BIT6)
#define CAMERA01_ON     (P4OUT &= ~BIT6)

#define CAMERA02_OFF    (P4OUT |=  BIT5)
#define CAMERA02_ON     (P4OUT &= ~BIT5)

#define CAMERA03_OFF    (P4OUT |=  BIT4)
#define CAMERA03_ON     (P4OUT &= ~BIT4)

#define CAMERA04_OFF    (P2OUT |=  BIT7)
#define CAMERA04_ON     (P2OUT &= ~BIT7)

#define FLASH_CS1_OFF   (P5OUT |=  BIT3)
#define FLASH_CS1_ON    (P5OUT &= ~BIT3)
#define FLASH_CS2_OFF   (P3OUT |=  BIT6)
#define FLASH_CS2_ON    (P3OUT &= ~BIT6)

/*
 * Init all GPIO and MCU subsystems
 */
void init_board()
{
    //Switch off everything on boot
    LED_OFF;
    CAMERA01_OFF;
    CAMERA02_OFF;
    CAMERA03_OFF;
    CAMERA04_OFF;
    MUX_OFF;

    //Configure the pinout

    //Configure LED on P1.0
    P1DIR |= BIT0;

    //Mux contollers for CAMs 3 and 4
    P2DIR |= BIT3;
    P2DIR |= BIT4;

    //Configure power pins
    P4DIR |= BIT4;
    P4DIR |= BIT5;
    P4DIR |= BIT6;
    P2DIR |= BIT7;

    // I2C pins
    P1SEL1 |=   BIT6 | BIT7;
    P1SEL0 &= ~(BIT6 | BIT7);

    P7SEL0 |=   BIT0 | BIT1;
    P7SEL1 &= ~(BIT0 | BIT1);

    //SPI pins
    P5SEL1 &= ~(BIT0 | BIT1 | BIT2);        // USCI_B1 SCLK, MOSI,
    P5SEL0 |= (BIT0 | BIT1 | BIT2);         // STE, and MISO pin
    PJSEL0 |= BIT4 | BIT5;// For XT1
    //CS1
    P5DIR |= BIT3;
    P5OUT |= BIT3;
    //CS2
    P3DIR |= BIT6;
    P3OUT |= BIT6;

    //TODO

    //Disable the GPIO Power-on default high-impedance mode to activate
    //previously configured port settings:
    PM5CTL0 &= ~LOCKLPM5;

    //Init clock to 8MHz using internal DCO, millis etc
    clock_init();

    //Init I2C
    //init_I2C();

    //Init SPI
    //init_SPI();

    //Open UART_DEBUG
    uart_init(UART_DEBUG, BR_9600);

    //TODO

    uint8_t i;
    for(i = 0; i < 10; i++)
    {
        LED_TOGGLE;
        __delay_cycles(400000);
    }
    LED_OFF;

    //Enable Interrupts
    _BIS_SR(GIE);
}

/**
 * main.c
 */
int main(void)
{
    //stop watchdog timer
	WDTCTL = WDTPW | WDTHOLD;

	//Init board hardware
	init_board();

	//Print reboot message on UART debug
	uart_print(UART_DEBUG, "IRIS2 is rebooting...\n");

	while(1)
	{
	    //Read Uptime:
	    uint64_t uptime = millis();

	    //Read UART Debug:
	    if(uart_available(UART_DEBUG) != 0)
	    {
	        uint8_t character = uart_read(UART_DEBUG);
	        if(character == 'a')
	            uart_print(UART_DEBUG, "Good!\n");
	        else
	            uart_print(UART_DEBUG, "Wrong command!\n");
	    }
	    //TODO

	    //Blink LED
	    if(uptime % 1000 > 100)
	        LED_OFF;
	    else
	        LED_ON;
	}
	
	return 0;
}