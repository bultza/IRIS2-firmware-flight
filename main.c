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
#include <stdio.h>
#include "configuration.h"
#include "clock.h"
#include "uart.h"
#include "i2c.h"
#include "i2c_TMP75C.h"
#include "i2c_DS1338Z.h"
#include "i2c_ADXL345.h"
#include "i2c_MS5611.h"
#include "i2c_INA.h"
#include "spi.h"
#include "spi_NOR.h"

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
    // For XT1
    PJSEL0 |= BIT4 | BIT5;
    //CS1
    FLASH_CS1_OFF;
    P5OUT |= BIT3;
    //CS2
    FLASH_CS2_OFF;
    P3OUT |= BIT6;

    //TODO

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
    configuration_init();

    //Open UART_DEBUG externally
    uart_init(UART_DEBUG, BR_9600);

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
	//uart_print(UART_DEBUG, "IRIS2 is rebooting...\n");

	///////////////////////////////////////////////////////////////////////////
	//DEBUG, KEEP THIS COMMENTED ON FLIGHT
	int16_t temperatures[6];
	struct RTCDateTime dateTime/* = {30, 07, 23, 24, 07, 21}*/; // Seconds, minute, hours, day, month, year
	int16_t accelerations[3];
	int32_t pressure;
	int32_t altitude;
	struct INAData inaData;
	i2c_TMP75_getTemperatures(temperatures);
	struct RDIDInfo dataRDID;
	//i2c_RTC_setClockData(&dateTime);
	//END OF DEBUG
	///////////////////////////////////////////////////////////////////////////

	uint64_t lastTime = 0;
	uint64_t uptime1 = 0;
	uint64_t uptime2 = 0;

	while(1)
	{
	    //Read Uptime:
	    uint64_t uptime = millis_uptime();

	    //UnixTime now:
	    uint32_t unixtTimeNow = i2c_RTC_unixTime_now();

	    //Read UART Debug:
	    if(uart_available(UART_DEBUG) != 0)
	    {
	        uint8_t character = uart_read(UART_DEBUG);
	        if(character == 'a')
	            uart_print(UART_DEBUG, "Good!\r\n");
	        else
	            uart_print(UART_DEBUG, "Wrong command!\r\n");
	    }

	    //Read once per second
	    if(uptime > lastTime + 1000)
	    {
	        i2c_TMP75_getTemperatures(temperatures);
	        i2c_RTC_getClockData(&dateTime);
	        //i2c_ADXL345_getAccelerations(accelerations);
	        uptime1 = millis_uptime();
	        i2c_MS5611_getPressure(&pressure);
	        uptime2 = millis_uptime();
	        i2c_MS5611_getAltitude(&pressure, &altitude);
	        i2c_INA_read(&inaData);
	        spi_NOR_getRDID(&dataRDID, CS_FLASH1);

	        char strToPrint[50];
	        sprintf(strToPrint, "Measuring pressure took %lld ms\r\n", uptime2-uptime1);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Temperature: %d\r\n", temperatures[0]);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Voltage: %d\r\n", inaData.voltage);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Current: %d\r\n", inaData.current);
	        uart_print(UART_DEBUG, strToPrint);

	        sprintf(strToPrint, "Second: %d\r\n", dateTime.seconds);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Minute: %d\r\n", dateTime.minutes);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Hour: %d\r\n", dateTime.hours);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Day: %d\r\n", dateTime.date);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Month: %d\r\n", dateTime.month);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Year: %d\r\n", dateTime.year);
	        uart_print(UART_DEBUG, strToPrint);

	        sprintf(strToPrint, "UNIXTIME: %ld\r\n", unixtTimeNow);
	        uart_print(UART_DEBUG, strToPrint);
	        /*
	         * * sprintf(strToPrint, "Acceleration X-Axis: %d\r\n", accelerations[0]);
	         * * uart_print(UART_DEBUG, strToPrint);
	         * * sprintf(strToPrint, "Acceleration Y-Axis: %d\r\n", accelerations[1]);
	         * * uart_print(UART_DEBUG, strToPrint);
	         * * sprintf(strToPrint, "Acceleration Z-Axis: %d\r\n", accelerations[2]);
	         * * uart_print(UART_DEBUG, strToPrint);
	         * * */
	        sprintf(strToPrint, "Pressure: %ld\r\n", pressure);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Altitude: %d\r\n", altitude);
	        uart_print(UART_DEBUG, strToPrint);

	        uart_print(UART_DEBUG, "\r\n");
	        lastTime = uptime;
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
