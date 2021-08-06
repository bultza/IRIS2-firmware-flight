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
    uart_init(UART_DEBUG, BR_115200);

    //Register that a boot happened just now:
    struct EventLine newEvent = {0};
    newEvent.upTime = (uint32_t) millis_uptime();
    newEvent.unixTime = i2c_RTC_unixTime_now();
    //newEvent.state = 0;
    //newEvent.sub_state = 0;
    newEvent.event = EVENT_BOOT;
    //Save the hardware reboot reason cause
    newEvent.payload[0] = SYSRSTIV_L;
    newEvent.payload[1] = SYSRSTIV_H;
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
	//uart_print(UART_DEBUG, "IRIS2 is rebooting...\n");

	///////////////////////////////////////////////////////////////////////////
	//DEBUG, KEEP THIS COMMENTED ON FLIGHT
	int16_t temperatures[6];
	struct RTCDateTime dateTime; //= {0, 42, 17, 3, 8, 21}; // Seconds, minute, hours, day, month, year
	//int16_t accelerations[3];
	int32_t pressure;
	int32_t altitude;
	struct INAData inaData;
	i2c_TMP75_getTemperatures(temperatures);
	struct RDIDInfo dataRDID;
	//i2c_RTC_setClockData(&dateTime);

	// Test of the NOR memory
	// Bulk erase the whole memory
	/*
	spi_NOR_bulkErase(CS_FLASH1);

	// Read 5 bytes from the beginning of the second sector (0x40000)
    static const uint32_t addressForOperations0 = (uint32_t) 0x00040000;
    uint8_t bufferToSaveRead0[5];
    spi_NOR_readFromAddress(addressForOperations0, bufferToSaveRead0, 5, CS_FLASH1);

    // Read 5 bytes from the beginning of the third sector (0x80000)
    static const uint32_t addressForOperations1 = (uint32_t) 0x00080000;
    uint8_t bufferToSaveRead1[5];
    spi_NOR_readFromAddress(addressForOperations1, bufferToSaveRead1, 5, CS_FLASH1);

	// Save AITOR at the beginning of the second sector (0x40000), then read
    uint8_t sentence0[5] = {'A','I','T','O','R'};
    spi_NOR_writeToAddress(addressForOperations0, sentence0, 5, CS_FLASH1);
    uint8_t bufferToSaveRead2[5];
    spi_NOR_readFromAddress(addressForOperations0, bufferToSaveRead2, 5, CS_FLASH1);

	// Now delete the sector, then read, then write RAMON, then read
    spi_NOR_eraseSector(addressForOperations0, CS_FLASH1);
    uint8_t bufferToSaveRead3[5];
    spi_NOR_readFromAddress(addressForOperations0, bufferToSaveRead3, 5, CS_FLASH1);
    uint8_t sentence1[5] = {'R','A','M','O','N'};
    spi_NOR_writeToAddress(addressForOperations0, sentence1, 5, CS_FLASH1);
    uint8_t bufferToSaveRead4[5];
    spi_NOR_readFromAddress(addressForOperations0, bufferToSaveRead4, 5, CS_FLASH1);

    // Write HELLO to the beginning of the third sector (0x8000), then read
    uint8_t sentence2[5] = {'H','E','L','L','O'};
    spi_NOR_writeToAddress(addressForOperations1, sentence2, 5, CS_FLASH1);
    uint8_t bufferToSaveRead5[5];
    spi_NOR_readFromAddress(addressForOperations1, bufferToSaveRead5, 5, CS_FLASH1);
    */

	// GoPro debug
	//cameraRawPowerOn(CAMERA01);

	//uint64_t cameraStepLastTime = 0;
	//uint8_t cameraStep= 0;

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
	    terminal_readAndProcessCommands();
	    /*
	    if(uart_available(UART_DEBUG) != 0)
	    {
	        uint8_t character = uart_read(UART_DEBUG);
	        if(character == 'a')
	            uart_print(UART_DEBUG, "Good!\r\n");
	        else
	            uart_print(UART_DEBUG, "Wrong command!\r\n");
	    }
	    */

	    /* TODO: CHECK FSM (RAMON)
	    //Run camera FSM continuously
	    cameraFSMcheck();

	    //Do something with the camera only once every 10s
	    if(uptime > cameraStepLastTime + 10000)
	    {
	        //Do something with the camera only once every 10s
	        if(cameraStep == 0)
	            cameraPowerOn(CAMERA01);
	        else if(cameraStep == 1)
	            cameraPowerOff(CAMERA01);

	        cameraStep++;
	        if(cameraStep > 1)
	            cameraStep = 0;
	        cameraStepLastTime = uptime;
	    }
	    */

	    //Read once per second
	    if(uptime > lastTime + 1000)
	    {
	        //char strToPrint[100];

	        i2c_TMP75_getTemperatures(temperatures);
	        i2c_RTC_getClockData(&dateTime);
	        //i2c_ADXL345_getAccelerations(accelerations);
	        uptime1 = millis_uptime();
	        i2c_MS5611_getPressure(&pressure);
	        uptime2 = millis_uptime();
	        i2c_MS5611_getAltitude(&pressure, &altitude);
	        i2c_INA_read(&inaData);
	        spi_NOR_getRDID(&dataRDID, CS_FLASH1);
	        //spi_NOR_getRDID(&dataRDID, CS_FLASH2);

	        /*sprintf(strToPrint, "Measuring pressure took %lld ms\r\n", uptime2-uptime1);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Temperature: %d\r\n", temperatures[0]);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Voltage: %d\r\n", inaData.voltage);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Current: %d\r\n", inaData.current);
	        uart_print(UART_DEBUG, strToPrint);

	        sprintf(strToPrint, "Date and time: %d/%d/%d %d:%d:%d\r\n", dateTime.date, dateTime.month, dateTime.year,
	                dateTime.hours, dateTime.minutes, dateTime.seconds);
            uart_print(UART_DEBUG, strToPrint);

	        sprintf(strToPrint, "UNIXTIME: %ld\r\n", unixtTimeNow);
	        uart_print(UART_DEBUG, strToPrint);
	        */

	        /*
	         * * sprintf(strToPrint, "Acceleration X-Axis: %d\r\n", accelerations[0]);
	         * * uart_print(UART_DEBUG, strToPrint);
	         * * sprintf(strToPrint, "Acceleration Y-Axis: %d\r\n", accelerations[1]);
	         * * uart_print(UART_DEBUG, strToPrint);
	         * * sprintf(strToPrint, "Acceleration Z-Axis: %d\r\n", accelerations[2]);
	         * * uart_print(UART_DEBUG, strToPrint);
	         * * */
	        /*sprintf(strToPrint, "Pressure: %ld\r\n", pressure);
	        uart_print(UART_DEBUG, strToPrint);
	        sprintf(strToPrint, "Altitude: %ld\r\n", altitude);
	        uart_print(UART_DEBUG, strToPrint);

	        uart_print(UART_DEBUG, "\r\n");*/
	        lastTime = uptime;
	        struct TelemetryLine newTelemetry = {0};
	        struct EventLine newEvent;
	        newTelemetry.upTime = uptime;
	        newTelemetry.unixTime = unixtTimeNow;
	        newTelemetry.altitude = altitude;
	        newTelemetry.temperatures[0] = temperatures[0];
	        saveTelemetry(newTelemetry);

	        //Try getting some of the already stored telemetry
	        getTelemetryFRAM(2, &newTelemetry);
	        getTelemetryFRAM(3, &newTelemetry);
	        getTelemetryFRAM(1, &newTelemetry);
	        getTelemetryFRAM(0, &newTelemetry);

	        getEventFRAM(0, &newEvent);
	        getEventFRAM(1, &newEvent);
	        getEventFRAM(2, &newEvent);
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
