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
 *      Ramon Garcia <ramongarciaalarcia@gmail.com>
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
 * P3.6 LED_R
 * P3.5 LED_G
 * P3.4 LED_B
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
 * P8.3 FLASH_CS2       //NOR Flash
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
#include <stdio.h>
#include "configuration.h"
#include "clock.h"
#include "uart.h"
#include "i2c.h"
#include "gopros.h"
#include "i2c_TMP75C.h"
#include "i2c_DS1338Z.h"
#include "i2c_ADXL345.h"
#include "i2c_MS5611.h"
#include "i2c_INA.h"
#include "spi.h"
#include "spi_NOR.h"
#include "datalogger.h"
#include "terminal.h"


#define LED_ON          (P1OUT |= BIT0)
#define LED_OFF         (P1OUT &= ~BIT0)
#define LED_TOGGLE      (P1OUT ^= BIT0)

#define LED_R_OFF       (P3OUT |= BIT6)
#define LED_R_ON        (P3OUT &= ~BIT6)
#define LED_R_TOGGLE    (P3OUT ^= BIT6)

#define LED_G_OFF       (P3OUT |= BIT5)
#define LED_G_ON        (P3OUT &= ~BIT5)
#define LED_G_TOGGLE    (P3OUT ^= BIT5)

#define LED_B_OFF       (P3OUT |= BIT4)
#define LED_B_ON        (P3OUT &= ~BIT4)
#define LED_B_TOGGLE    (P3OUT ^= BIT4)

/*
 * Init all GPIO and MCU subsystems
 */
void init_board()
{
    //Switch off everything on boot
    LED_OFF;
    LED_R_OFF;
    LED_G_OFF;
    LED_B_OFF;
    CAMERA01_OFF;
    CAMERA02_OFF;
    CAMERA03_OFF;
    CAMERA04_OFF;
    MUX_OFF;

    //Configure the pinout

    //Configure LED on P1.0
    P1DIR |= BIT0;

    //Configure LED on P3.6, 5, 4
    P3DIR |= BIT4 | BIT5 | BIT6;

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

    //CS1
    FLASH_CS1_OFF;
    P5DIR |= BIT3;
    //CS2
    FLASH_CS2_OFF;
    P8DIR |= BIT3;

    // For XT1
    PJSEL0 |= BIT4 | BIT5;

    //Disable the GPIO Power-on default high-impedance mode to activate
    //previously configured port settings:
    PM5CTL0 &= ~LOCKLPM5;

    //Init clock to 8MHz using internal DCO, millis etc
    clock_init();

    //Init I2C
    i2c_master_init();

    //Init SPI
    spi_init(CR_8MHZ);

    //Init Temperature sensor
    i2c_TMP75_init();

    //Init RTC
    i2c_RTC_init();

    //Init Accelerometer
    //i2c_ADXL345_init();

    //Init Barometer
    i2c_MS5611_init();

    //Init INA
    i2c_INA_init();

    //Init NOR Memory
    //TODO

    //Init configuration
    int8_t error = configuration_init();

    if(error != 0)
    {
        //Search where in the NOR we should continue
        searchAddressesNOR();
    }

    //Open UART_DEBUG externally
    uart_init(UART_DEBUG, BR_115200);

    //Register that a boot happened just now:
    struct EventLine newEvent = {0};
    newEvent.upTime = (uint32_t) millis_uptime();
    newEvent.unixTime = i2c_RTC_unixTime_now();
    //newEvent.state = 0;
    //newEvent.sub_state = 0;
    newEvent.event = EVENT_BOOT;
    //Save the hardware reboot reason cause
    confRegister_.hardwareRebootReason = SYSRSTIV;
    newEvent.payload[0] = confRegister_.hardwareRebootReason & 0xFF;
    newEvent.payload[1] = (uint8_t) 0xFF & ((confRegister_.hardwareRebootReason & 0xFF00) >> 8);
    newEvent.payload[4] = FWVERSION; //Adding firmware version!!
    //Clear system reset interrupt vector
    SYSRSTIV = 0;
    saveEvent(newEvent);

    //Make 5 blinks to recognize the boot process:
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
	uart_print(UART_DEBUG, "\r\nIRIS2 is booting up...\r\n");

	///////////////////////////////////////////////////////////////////////////
	//DEBUG, KEEP THIS COMMENTED ON FLIGHT
	char strLine[100];
	uint16_t size = (uint32_t)sizeof(struct TelemetryLine);
	sprintf(strLine, "Size of TelemetryLine is %d\r\n", size);
	uart_print(UART_DEBUG, strLine);
	size = (uint32_t)sizeof(struct EventLine);
    sprintf(strLine, "Size of EventLine is %d\r\n", size);
    uart_print(UART_DEBUG, strLine);
	//TODO
	//END OF DEBUG
	///////////////////////////////////////////////////////////////////////////

	while(1)
	{
	    //Read Uptime:
	    uint64_t uptime = millis_uptime();

	    //UnixTime now:
	    uint32_t unixtTimeNow = i2c_RTC_unixTime_now();

	    //Read UART Debug:
	    terminal_readAndProcessCommands();

	    //Run camera FSM continuously
	    cameraFSMcheck();

	    //Read all sensors
	    sensors_read();

	    //Save telemetry periodically
	    saveTelemetry();

	    //Fligh sequence
	    //TODO

	    //Blink LED
	    if(uptime % 1000 > 100)
	        LED_OFF;
	    else
	        LED_ON;

	    //Blink LED
        if((uptime + 100) % 1000 > 100)
            LED_R_OFF;
        else
            LED_R_ON;

        //Blink LED
        if((uptime + 200) % 1000 > 100)
            LED_G_OFF;
        else
            LED_G_ON;

        //Blink LED
        if((uptime + 300) % 1000 > 100)
            LED_B_OFF;
        else
            LED_B_ON;
	};
	
	return 0;
}
