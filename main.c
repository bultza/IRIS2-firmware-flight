
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
#include "flight_sequence.h"
#include "leds.h"

/*
 * Init all GPIO and MCU subsystems
 */
void init_board()
{
    //Switch off everything on boot
    led_off();
    led_g_off();
    led_r_off();
    led_b_off();
    CAMERA01_OFF;
    CAMERA02_OFF;
    CAMERA03_OFF;
    CAMERA04_OFF;
    MUX_OFF;

    //Configure the pinout

    //Configure LED on P1.0
    P1DIR |= BIT0;

    //Sunrise Flight Signal
    P2DIR &= ~BIT2;
    //Enable pull down or up resistor
    P2REN |= BIT2;
    //Configure as pull-up resistor
    P2OUT |= BIT2;

    //Configure LED on P3.6, 5, 4
    P3DIR |= BIT4 | BIT5 | BIT6;

    //Configure interrupt from ACC as input
    P3DIR &= ~BIT7;

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

    //WDT Enabled
    WDTCTL = WDTPW | DAE_WDTKICK;

    //Enable Interrupts
    _BIS_SR(GIE);

    //Init I2C
    i2c_master_init();

    //Init SPI
    spi_init(CR_8MHZ);

    //Init Temperature sensor
    i2c_TMP75_init();

    //Init RTC
    i2c_RTC_init();

    //Init Accelerometer
    i2c_ADXL345_init();

    //Init Barometer
    i2c_MS5611_init();

    //Init INA
    i2c_INA_init();

    //Init configuration
    int8_t error = configuration_init();

    //Init NOR Memory
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
        led_toggle();
        led_g_toggle();
        __delay_cycles(400000);
    }
    led_off();;
    led_g_off();
}

uint8_t ledBlinkCounter_ = 0;
uint8_t lastLEDStatus_ = 0;
/**
 * If everything is ok it will blink once per second
 * If baro error it blinks twice as fast
 */
void ledDebugBlink(uint64_t uptime)
{
    uint16_t blinkPeriod = 1000;
    if(getBaroIsOnError())
        blinkPeriod = 250;

    //If debug, blink always
    if(confRegister_.flightState == FLIGHTSTATE_DEBUG)
    {
        if(uptime % blinkPeriod > 50)
            led_off();
        else
            led_on();
        return;
    }

    //If not debug, blink as many times as the flight status
    if(uptime % blinkPeriod > 50)
    {
        if(lastLEDStatus_ == 1)
        {
            //We have the transition here
            if(ledBlinkCounter_ > confRegister_.flightState)
                ledBlinkCounter_ = 0;
        }
        led_off();
        lastLEDStatus_ = 0;
    }
    else
    {
        if(lastLEDStatus_ == 0)
        {
            //We have the transition here
            ledBlinkCounter_++;
        }
        if(ledBlinkCounter_ != confRegister_.flightState + 1)
            led_on();
        lastLEDStatus_ = 1;
    }
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

#ifdef DEBUG_MODE
	uart_print(UART_DEBUG, "*******************************************************************************\r\n");
	uart_print(UART_DEBUG, "*WARNING! IRIS2 IS IN DEBUG MODE, YOU SHOULD NOT FLIGHT WITH THIS COMPILATION!*\r\n");
	uart_print(UART_DEBUG, "*******************************************************************************\r\n");
#endif

	///////////////////////////////////////////////////////////////////////////
	//DEBUG, KEEP THIS COMMENTED ON FLIGHT
	/*char strLine[100];
	uint16_t size = (uint32_t)sizeof(struct TelemetryLine);
	sprintf(strLine, "Size of TelemetryLine is %d\r\n", size);
	uart_print(UART_DEBUG, strLine);
	size = (uint32_t)sizeof(struct EventLine);
    sprintf(strLine, "Size of EventLine is %d\r\n", size);
    uart_print(UART_DEBUG, strLine);
	*/
	//END OF DEBUG
	///////////////////////////////////////////////////////////////////////////

    //Terminal begin by default thanks
    terminal_start();

	while(1)
	{
	    //Kick WDT
        WDTCTL = WDTPW | DAE_WDTKICK;

	    //Read Uptime:
	    uint64_t uptime = millis_uptime();

	    //UnixTime now:
	    uint32_t unixtTimeNow = i2c_RTC_unixTime_now();

	    //Read UART Debug:
	    terminal_readAndProcessCommands();

	    //Run camera FSM continuously
	    cameraFSMcheck();

	    //Read all sensors
	    sensorsRead();

	    //Save telemetry periodically
	    saveTelemetry();

	    //Save telemetry periodically
        checkMemory();

        //Save telemetry periodically
        checkFlightSignal();

	    //Fligh sequence
	    checkFlightSequence();

	    //Blink CPU LED
	    ledDebugBlink(uptime);

	    //Blink FP Green LED once per 5s
        if(uptime % 5000 > 10)
            led_g_off();
        else
            led_g_on();

        //Keep the Red LED On if there is power on the cameras
        if((P4OUT & BIT6) &&
                (P4OUT & BIT5) &&
                (P4OUT & BIT4) &&
                (P2OUT & BIT7))
            led_r_off();
        else
            led_r_on();
	};

	//It should never reach here
}
